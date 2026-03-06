import os
from langchain_core.prompts import ChatPromptTemplate
import chromadb
import utils

def c_func_embed(input):
    """
    Embedding function compatible with ChromaDB.
    Handles both single string (query) and list of strings (documents).
    """

    if isinstance(input, str):
        return utils.embeddings_model.embed_query(input)
    elif isinstance(input, list):
        return utils.embeddings_model.embed_documents(input)
    else:
        # Fallback for other iterables
        return utils.embeddings_model.embed_documents(list(input))

def init_knowledge_base():
    chroma_client = chromadb.Client()
    collection = chroma_client.get_or_create_collection(
        name="code_optimizations",
        embedding_function=c_func_embed
    )
    if collection.count() != 0:
        print("Knowledge base already initialized.")
        return collection

    print("Initializing knowledge base...")
    documents = []
    metadatas = []
    
    # Ensure knowledge_base_root exists
    for repo_name in os.listdir(utils.knowledge_base_root):
        commits_path = os.path.join(utils.knowledge_base_root, repo_name, "modified_file")
        for commit_hash in os.listdir(commits_path):
            commit = utils.CommitInfo(utils.knowledge_base_root, repo_name, commit_hash)
            documents.append(commit.origin_func)
            metadatas.append({"commit_info": commit})

    if documents:
        # Generate IDs as strings
        ids = [str(i) for i in range(len(documents))]
        collection.add(documents=documents, metadatas=metadatas, ids=ids)
        print(f"Added {len(documents)} documents.")
    else:
        print("No documents found to add.")
        
    return collection

knowledge_base = init_knowledge_base()

def query_context(mission: utils.Mission) -> str:
    try:
        similar = knowledge_base.query(
            query_texts=[mission.origin_func],
            n_results=5
        )
    except Exception as e:
        print(f"RAG Query failed: {e}")
        similar = None

    similar = map(lambda x: x['metadata']['commit_info'], similar['metadatas'][0])
    similar = filter(lambda x: x.repo_name != mission.repo_name, similar)
    similar = list(similar[:3].reverse())
    # at most 3 similar examples from different repos,
    # reverse to have most similar last for better CoT reasoning

    if len(similar) == 0:
        print("[Warning] No similar examples found in knowledge base for mission {mission.id}.")
        return ""

    context = f"Below are {len(similar)} reference examples\n\n"
    for num, example in enumerate(similar):
        context += f"Example {num+1} (Repo: {example.repo_name}, Commit: {example.commit_hash}):\n"
        context += f"Original Function:\n```{example.lang}\n{example.origin_func}\n```\n"
        context += f"Optimized Function:\n```{example.lang}\n{example.target_func}\n```\n\n"
    return context

def optimize(mission: utils.Mission) -> tuple[str, str]:
    """
    Optimizes the provided C/C++ code using an LLM with Retrieval-Augmented Generation (RAG).
    """

    rag_context = query_context(mission)

    system_prompt = (
        "You are an expert C/C++ optimization engineer. "
        "Your goal is to optimize a specific function within the provided code for maximum runtime efficiency and minimal resource usage.\n"
        "Do NOT strictly focus on readability or style; focus on performance.\n\n"
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
        response = chain.invoke({"user_prompt": user_content})
        content = response.content
        return utils.parse_final_output(content)
    except Exception as e:
        print(f"Error during optimization: {e}")
        raise e
