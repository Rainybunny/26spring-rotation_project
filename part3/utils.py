import os
import re
import sys
from pathlib import Path
import chromadb

# Add project root to sys.path to allow importing config
sys.path.append(str(Path(__file__).parent.parent.absolute()))
import config

from langchain_openai import ChatOpenAI, OpenAIEmbeddings

# shared large-language model api definition for judgment side
judge_llm = ChatOpenAI(
    openai_api_key=config.xmcp_api_key,
    openai_api_base=config.xmcp_base_url,
    model_name=config.xmcp_juedge_model,
    temperature=0.0  # Lower temperature for deterministic judgment
)

# shared large-language model api definition for optimization side
llm = ChatOpenAI(
    openai_api_key=config.xmcp_api_key,
    openai_api_base=config.xmcp_base_url,
    model_name=config.xmcp_model,
    temperature=0.0  # Lower temperature for deterministic code generation
)

embeddings_model = OpenAIEmbeddings(
    openai_api_key=config.xmcp_api_key,
    openai_api_base=config.xmcp_base_url,
    model=config.xmcp_embedding_model,
    check_embedding_ctx_length=False
)

class MyEmbeddingFunction(chromadb.EmbeddingFunction):
    def __call__(self, input):
        if isinstance(input, str):
            return embeddings_model.embed_query(input)
        else:
            return embeddings_model.embed_documents(list(input))

project_root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
benchmark_root = os.path.join(project_root, "benchmark")
knowledge_base_root = os.path.join(project_root, "knowledge_base")
test_root = os.path.join(project_root, "part3", "tests")

class CommitInfo:
    def __init__(self, root_path, repo_name, commit_hash):
        commit_folder = os.path.join(root_path, repo_name, "modified_file", commit_hash)
        before_file = None
        after_file = None
        before_func = None
        after_func = None
        for file in os.listdir(commit_folder):
            if file.startswith("before."):
                before_file = os.path.join(commit_folder, file)
            elif file.startswith("after."):
                after_file = os.path.join(commit_folder, file)
            elif file.startswith("before_func."):
                before_func = os.path.join(commit_folder, file)
            elif file.startswith("after_func."):
                after_func = os.path.join(commit_folder, file)
        self.repo_name = repo_name
        self.commit_hash = commit_hash
        self.origin = open(before_file, 'r').read()
        self.target = open(after_file, 'r').read()
        self.origin_func = open(before_func, 'r').read()
        self.target_func = open(after_func, 'r').read()
        self.suffix = after_file.split('.')[-1]
        self.lang = 'cpp' if self.suffix in ['cpp', 'cc', 'cxx', 'hpp'] else 'c'

class Mission(CommitInfo):
    def __init__(self, repo_name, commit_hash, test_folder):
        super().__init__(benchmark_root, repo_name, commit_hash)

        trunked_commit_hash = commit_hash[:3] + commit_hash[-3:]
        output_name = f"{repo_name}_{trunked_commit_hash}.{self.suffix}"
        self.output_file = os.path.join(test_folder, output_name)

        self.opt_prompt = None # optimization prompt
        self.output = None # output content
        self.opt_reason = None # optimization reason
        self.result = None # judge result
        self.judge_reason = None # judge reason

    def show_goal(self):
        res = f"\nFull source code before optimization:\n"
        res += f"```{self.lang}\n{self.origin}\n```\n\n"
        res += f"The only function in the code above that is required for optimization:\n"
        res += f"```{self.lang}\n{self.origin_func}\n```\n\n"
        return res

    def show_result(self):
        res = f"\nFull source code before optimization:\n"
        res += f"```{self.lang}\n{self.origin}\n```\n\n"
        res += f"The only function in the code above that is optimized:\n"
        res += f"```{self.lang}\n{self.output}\n```\n\n"
        return res

system_prompt_role_prelog = (
    "You are an expert C/C++ optimization engineer. "
    "Your goal is to optimize a specific function within the provided code for maximum runtime efficiency and minimal resource usage.\n"
    "Do NOT strictly focus on readability or style; focus on performance.\n"
    "Do not introduce any uncertain behavior; do not cause any behavior-change out of the targeted function.\n\n"
)

system_prompt_final_output_format = (
    "First, output your thought process/analysis.\n"
    "Second, output a clear description of the final optimization idea.\n"
    "Then, output the final optimized COMPLETE FUNCTION IMPLEMENTATION inside a single markdown code block (```cpp ... ``` or ```c ... ```).\n"
    "The code block must output full code snippet that can be an exact replacement for the given original function code."
)

def parse_final_output(content: str) -> tuple[str, str]:
    # Extract code block
    pattern = r"```(?:c|cpp|C\+\+|cc)?\s*\n(.*?)```"
    matches = list(re.finditer(pattern, content, re.DOTALL | re.IGNORECASE))

    if matches:
        last_match = matches[-1]
        code = last_match.group(1).strip()
        reason = content[:last_match.start()].strip()
        return code, reason

    # Fallback generic block
    pattern_generic = r"```\s*\n(.*?)```"
    matches_generic = list(re.finditer(pattern_generic, content, re.DOTALL))
    
    if matches_generic:
        last_match = matches_generic[-1]
        code = last_match.group(1).strip()
        reason = content[:last_match.start()].strip()
        return code, reason

    return content, "[Warning] No code block found in response."
