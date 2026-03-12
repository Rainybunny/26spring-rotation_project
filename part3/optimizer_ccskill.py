import json
import os
import re
import concurrent.futures
from langchain_core.prompts import ChatPromptTemplate
from langchain_core.output_parsers import StrOutputParser
import utils

class Strategy:
    """
    Represents an optimization strategy.
    
    Attributes:
        id (str): The unique identifier for the strategy.
        summary (str): A brief description of the strategy.
        examples (list): A list of dictionaries containing 'repo' and 'hash' for reference examples.
    """
    def __init__(self, s_id: str, summary: str, examples: list):
        self.id = s_id
        self.summary = summary
        self.examples = examples

    def to_dict(self):
        return {
            "id": self.id,
            "summary": self.summary,
            "examples": self.examples
        }

    @classmethod
    def from_dict(cls, data):
        return cls(data["id"], data["summary"], data["examples"])

    def get_details(self) -> str:
        """
        Retrieves the detailed strategy prompt content, including examples.
        """
        details = f"Selected Strategy: {self.summary}\n\n"
        details += "Reference Examples:\n"
        
        for i, ex in enumerate(self.examples):
            try:
                commit_info = utils.CommitInfo(utils.knowledge_base_root, ex['repo'], ex['hash'])
                details += f"\nExample {i+1} (Repo: {ex['repo']}, Commit: {ex['hash']}):\n"
                details += f"Original Function:\n```{commit_info.lang}\n{commit_info.origin_func}\n```\n"
                details += f"Optimized Function:\n```{commit_info.lang}\n{commit_info.target_func}\n```\n"
                details += f"Optimization Summary: {self.summary}\n"
            except Exception as e:
                print(f"[Warning] Could not load example {ex['repo']}/{ex['hash']}: {e}")
                continue
        return details

# Global storage for strategies
STRATEGIES: dict[str, Strategy] = {}
STRATEGIES_FILE = os.path.join(utils.project_root, "data", "strategies.json")

def generate_cluster_summary(cluster_id: str, commit_summaries: list[str]) -> str:
    """
    Uses LLM to generate a consolidated summary for a cluster of optimization commits.
    """
    # Deduplicate and limit summaries to avoid hitting token limits
    unique_summaries = list(set(commit_summaries))[:10]
    
    system_prompt = (
        "You are an expert code optimization analyst. "
        "Your task is to synthesize a single, concise, and actionable optimization strategy description "
        "from a list of individual commit summaries belonging to the same cluster.\n"
        "The summary should be general enough to apply to similar code patterns but specific enough to be useful.\n"
        "Output ONLY the summary string."
    )
    
    user_prompt = "Commit Summaries:\n" + "\n".join([f"- {s}" for s in unique_summaries])
    
    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{user_prompt}")
    ])
    
    chain = prompt | utils.llm | StrOutputParser()
    
    try:
        return chain.invoke({"user_prompt": user_prompt}).strip()
    except Exception as e:
        print(f"[Warning] Failed to generate summary for cluster {cluster_id}: {e}")
        return unique_summaries[0] if unique_summaries else "General optimization."

def process_cluster(cluster):
    c_id = str(cluster["cluster_id"])
    
    # Collect summaries
    summaries = []
    commits = cluster.get("commits", [])
    
    for c in commits:
        s = c.get("optimization_summary_final", "")
        if s:
            summaries.append(s)
        elif c.get("optimization_summary"):
            if isinstance(c["optimization_summary"], list):
                summaries.extend(c["optimization_summary"])
            else:
                summaries.append(c["optimization_summary"])
                
    if not summaries:
        return None

    # Generate consolidated summary using LLM
    summary = generate_cluster_summary(c_id, summaries)
    
    # Select examples
    examples = []
    
    # 1. Add most representative commit if available
    mrc = cluster.get("most_representative_commit")
    if mrc:
        examples.append({
            "repo": mrc["repository_name"],
            "hash": mrc["hash"]
        })
    
    # 2. Add another high-scoring commit for diversity (that isn't the MRC)
    commits.sort(key=lambda x: x.get("representativeness_score", 0), reverse=True)
    for c in commits:
        if len(examples) >= 2:
            break
        # Avoid duplicates
        is_duplicate = False
        for ex in examples:
            if ex["repo"] == c["repository_name"] and ex["hash"] == c["hash"]:
                is_duplicate = True
                break
        
        if not is_duplicate:
            examples.append({
                "repo": c["repository_name"],
                "hash": c["hash"]
            })
            
    return Strategy(c_id, summary, examples)

def init_strategy_base():
    """
    Initializes the strategy knowledge base.
    Tries to load from 'strategies.json' first.
    If not found, builds from '0_76.json' and saves to 'strategies.json'.
    """
    global STRATEGIES
    if STRATEGIES:
        return

    # Try loading from persisted file
    if os.path.exists(STRATEGIES_FILE):
        try:
            with open(STRATEGIES_FILE, 'r') as f:
                data = json.load(f)
                for s_data in data:
                    strategy = Strategy.from_dict(s_data)
                    STRATEGIES[strategy.id] = strategy
            print(f"Loaded {len(STRATEGIES)} strategies from {STRATEGIES_FILE}.")
            return
        except Exception as e:
            print(f"[Warning] Failed to load strategies from file: {e}. Rebuilding...")

    # Build from clustering results
    json_path = os.path.join(utils.project_root, "data", "0_76.json")
    if not os.path.exists(json_path):
        print(f"[Warning] Clustering source file not found: {json_path}")
        return

    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
        
        clusters = data.get("clusters", [])
        print(f"Generating strategies for {len(clusters)} clusters... This might take a while.")
        
        # Parallel processing for LLM calls
        new_strategies = []
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            future_to_cluster = {executor.submit(process_cluster, cluster): cluster for cluster in clusters}
            for future in concurrent.futures.as_completed(future_to_cluster):
                try:
                    strategy = future.result()
                    if strategy:
                        new_strategies.append(strategy)
                except Exception as exc:
                    print(f"Cluster processing generated an exception: {exc}")

        # Sort and store
        new_strategies.sort(key=lambda s: int(s.id) if s.id.isdigit() else s.id)
        for s in new_strategies:
            STRATEGIES[s.id] = s
        
        # Save to file for reuse
        try:
            with open(STRATEGIES_FILE, 'w') as f:
                json.dump([s.to_dict() for s in STRATEGIES.values()], f, indent=2)
            print(f"Initialized and saved {len(STRATEGIES)} strategies.")
        except Exception as e:
            print(f"[Error] Failed to save strategies: {e}")
            
    except Exception as e:
        print(f"[Error] Failed to initialize strategies: {e}")

# Initialize strategies when module is loaded
init_strategy_base()

def get_strategy_catalog_text():
    """
    Generates a text representation of the strategy catalog for the LLM.
    """
    lines = []
    try:
        sorted_ids = sorted(STRATEGIES.keys(), key=lambda x: int(x))
    except ValueError:
        sorted_ids = sorted(STRATEGIES.keys())
        
    for s_id in sorted_ids:
        lines.append(f"Strategy {s_id}: {STRATEGIES[s_id].summary}")
    return "\n".join(lines)

def select_strategy(mission: utils.Mission) -> str:
    """
    Phase 1: Select the most applicable strategy from the catalog.
    Returns the strategy ID or None.
    """
    catalog = get_strategy_catalog_text()
    if not catalog:
        print("Strategy catalog is empty.")
        return None

    system_prompt = (
        "You are an expert code optimization strategist. "
        "Your goal is to analyze the provided C/C++ code and select the most suitable optimization strategy from the given catalog.\n"
        "1. Analyze the code for performance bottlenecks.\n"
        "2. Review the provided list of optimization strategies.\n"
        "3. Select the ONE strategy that is most applicable and likely to provide the best speedup.\n"
        "4. If none of the strategies are clearly applicable, return 'None'.\n\n"
        "Output Format:\n"
        "Reasoning: <brief explanation of why this strategy applies>\n"
        "Strategy ID: <output ONLY the ID, e.g. '12', or 'None'>"
    )
    
    user_prompt = f"Strategy Catalog:\n{catalog}\n\nTask Code:\n```{mission.lang}\n{mission.origin_func}\n```"

    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{user_prompt}")
    ])
    
    chain = prompt | utils.llm | StrOutputParser()
    
    try:
        response = chain.invoke({"user_prompt": user_prompt})
        
        # improved parsing logic
        match = re.search(r"Strategy ID:\s*(None|\d+)", response, re.IGNORECASE)
        if match:
            s_id = match.group(1).strip()
            if s_id.lower() == "none":
                return None
            return s_id
            
        return None
    except Exception as e:
        print(f"Error during strategy selection: {e}")
        return None

def optimize(mission: utils.Mission) -> tuple[str, str]:
    """
    Optimizes the provided C/C++ code using the selected strategy.
    """
    
    # Phase 1: Select Strategy
    selected_strategy_id = select_strategy(mission)
    
    if selected_strategy_id and selected_strategy_id in STRATEGIES:
        strategy = STRATEGIES[selected_strategy_id]
        strategy_details = strategy.get_details()
    else:
        # Fallback handling could go here
        print(f"[Warning] No applicable strategy selected for mission\n"
              f"> {mission.repo_name}/{mission.commit_hash}.")
        strategy_details = None

    system_prompt = (
        f"{utils.system_prompt_role_prelog}"
        "You MUST use Chain of Thought (CoT) reasoning:\n"
        "1. Analyze the original code and the specific function to identify bottlenecks.\n"
        "2. If a specific strategy was selected (with examples), analyze how it applies to the current task. "
        "Explain WHY it applies and HOW you will adapt the technique.\n" 
        "3. Propose optimizations. Prioritize simple, high-impact changes consistent with the selected strategy.\n"
        "4. Select EXACTLY ONE optimization idea to apply.\n"
        "5. Apply the chosen optimization to the function.\n\n"
        "Output Format:\n"
        f"{utils.system_prompt_final_output_format}"
    )
    
    user_content = ""
    if strategy_details is not None:
        user_content += f"{strategy_details}\n\n"
        user_content += "Based on the selected strategy and reference examples above, "
        user_content += "apply the same optimization principle to the current task.\n"
    
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
        print(f"Error during optimization phase: {e}")
        # Return something to handle the error gracefully
        return mission.origin_func, f"Error: {e}"
