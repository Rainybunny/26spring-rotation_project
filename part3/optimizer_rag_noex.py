import os
from langchain_core.prompts import ChatPromptTemplate
import chromadb
import utils

def init_knowledge_base():
    persist_directory = os.path.join(utils.project_root, "chroma_db")
    chroma_client = chromadb.PersistentClient(path=persist_directory)
    collection = chroma_client.get_or_create_collection(
        name="code_optimizations",
        embedding_function=utils.MyEmbeddingFunction()
    )
    if collection.count() != 0:
        print("Knowledge base loaded from disk.")
        return collection

    print("Initializing knowledge base...")
    documents = []
    metadatas = []
    
    # Ensure knowledge_base_root exists
    for repo_name in os.listdir(utils.knowledge_base_root):
        commits_path = os.path.join(utils.knowledge_base_root, repo_name, "modified_file")
        if not os.path.exists(commits_path):
            continue # repos without modified_file folder are ignored
        for commit_hash in os.listdir(commits_path):
            commit = utils.CommitInfo(utils.knowledge_base_root, repo_name, commit_hash)
            documents.append(commit.origin_func)
            metadatas.append({"repo_name": repo_name, "commit_hash": commit_hash})

    if documents:
        # Generate IDs as strings
        ids = [str(i) for i in range(len(documents))]

        def add_batch_safe(batch_docs, batch_metas, batch_ids):
            try:
                collection.add(
                    documents=batch_docs,
                    metadatas=batch_metas,
                    ids=batch_ids
                )
            except Exception as e:
                # If a single document failed, drop it
                if len(batch_docs) == 1:
                    print(f"Skipping document {batch_ids[0]} due to error: {e}")
                    return
                
                # Otherwise, split and retry
                mid = len(batch_docs) // 2
                add_batch_safe(batch_docs[:mid], batch_metas[:mid], batch_ids[:mid])
                add_batch_safe(batch_docs[mid:], batch_metas[mid:], batch_ids[mid:])
        
        # Batch adding documents to avoid RateLimitError and ContextLengthExceeded
        batch_size = 5  # Small batch size to stay within token limits
        for i in range(0, len(documents), batch_size):
            batch_end = min(i + batch_size, len(documents))
            print(f"Adding batch {i} to {batch_end}...")
            add_batch_safe(
                documents[i:batch_end],
                metadatas[i:batch_end],
                ids[i:batch_end]
            )
        print(f"Added {len(documents)} documents (some might have been skipped).")
    else:
        print("No documents found to add.")

    return collection

knowledge_base = init_knowledge_base()
wanted_example_num = 4
# Chroma returns distance (smaller is more similar). Keep a relaxed cap so we still
# provide some context on hard cases while filtering obvious outliers.
distance_threshold = 1.2

def query_context(mission: utils.Mission) -> str:
    try:
        similar = knowledge_base.query(
            query_texts=[mission.origin_func],
            n_results=20
        )
    except Exception as e:
        print(f"RAG Query failed: {e}")
        return ""

    distances = similar.get("distances", [[]])[0]
    metadatas = similar.get("metadatas", [[]])[0]
    if not distances or not metadatas:
        print(f"[Warning] No retrieval candidates for mission\n"
              f"> {mission.repo_name}/{mission.commit_hash}.")
        return ""

    # Keep nearest neighbors first and diversify by repository to avoid nearly
    # duplicate examples from a single codebase.
    pairs = sorted(zip(distances, metadatas), key=lambda x: x[0])
    selected_meta = []
    used_repos = set()
    for distance, metadata in pairs:
        repo_name = metadata.get("repo_name")
        commit_hash = metadata.get("commit_hash")
        if not repo_name or not commit_hash:
            continue
        if repo_name == mission.repo_name and commit_hash == mission.commit_hash:
            continue
        if distance > distance_threshold:
            continue
        if repo_name in used_repos:
            continue
        selected_meta.append((distance, repo_name, commit_hash))
        used_repos.add(repo_name)
        if len(selected_meta) >= wanted_example_num:
            break

    if not selected_meta:
        print(f"[Warning] No similar examples found in knowledge base for mission\n"
              f"> {mission.repo_name}/{mission.commit_hash}.")
        return ""

    examples = []
    for distance, repo_name, commit_hash in selected_meta:
        try:
            examples.append((distance, utils.CommitInfo(utils.knowledge_base_root, repo_name, commit_hash)))
        except Exception as e:
            print(f"[Warning] Skip broken example {repo_name}/{commit_hash}: {e}")

    if not examples:
        return ""

    context = f"Below are {len(examples)} reference examples\n\n"
    for num, (distance, example) in enumerate(examples):
        context += f"Example {num+1} (Repo: {example.repo_name}, Commit: {example.commit_hash}):\n"
        context += f"Similarity distance: {distance:.4f}\n"
        context += f"Original Function:\n```{example.lang}\n{example.origin_func}\n```\n"
        context += f"Optimized Function:\n```{example.lang}\n{example.target_func}\n```\n\n"

    if len(examples) < wanted_example_num:
        print(f"[Info] Using {len(examples)} retrieved examples for mission\n"
              f"> {mission.repo_name}/{mission.commit_hash}.")
    return context

def optimize(mission: utils.Mission) -> tuple[str, str]:
    """
    Optimizes the provided C/C++ code using an LLM with Retrieval-Augmented Generation (RAG).
    """

    rag_context = query_context(mission)

    system_prompt = (
        f"{utils.system_prompt_role_prelog}"
        "You MUST use Chain of Thought (CoT) reasoning:\n"
        "1. Analyze the original code and the specific function to identify bottlenecks.\n"
        "2. If some reference examples are provided, analyze how it was optimized and if similar techniques apply.\n"
        "3. Propose optimizations. Prioritize simple, high-impact changes.\n"
        "4. Select EXACTLY ONE optimization idea to apply.\n"
        "5. Apply the chosen optimization to the function.\n\n"
        "Output Format:\n"
        f"{utils.system_prompt_final_output_format}"
    )
    
    user_content = ""
    if rag_context:
        user_content += f"{rag_context}\n\n"
        user_content += "Based on the reference examples above, "
        user_content += "analyze if any of the optimization techniques used in those examples "
        user_content += "can be applied to the current task. "
        user_content += "If so, explain which technique you will apply and why.\n\n"
    
    user_content += f"Task:\n{mission.show_goal()}"
    
    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{user_prompt}")
    ])
    
    chain = prompt | utils.llm
    
    try:
        mission.opt_prompt = (
            f"[SYSTEM]\n{system_prompt}\n\n"
            f"[USER]\n{user_content}"
        )
        response = chain.invoke({"user_prompt": user_content})
        content = response.content
        return utils.parse_final_output(content)
    except Exception as e:
        print(f"Error during optimization: {e}")
        raise e
