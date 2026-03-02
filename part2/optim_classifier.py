import openai
import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent))
import config
import commit_retrieve as cr

client = openai.OpenAI(
    api_key = config.xmcp_api_key,
    base_url = config.xmcp_base_url # LiteLLM Proxy is OpenAI compatible, Read More: https://docs.litellm.ai/docs/proxy/user_keys
)

system_prompt = "You are an AI assistant specialized in analyzing code changes from git commits. Your task is to determine whether a given commit qualifies as a 'code optimization'. Code optimization here strictly means changes that improve code runtime efficiency (e.g., faster execution) or reduce resource consumption (e.g., memory, CPU, network). It does NOT include changes solely aimed at improving code readability, maintainability, style, or documentation. You will be provided with the commit message and a list of file changes, each containing file path, status (modified/added/deleted), and the diff (changes). Based on this information, decide if the commit is an optimization. Respond with exactly one word: 'true' if it is an optimization, 'false' if it is not, or 'unknown' if you cannot determine confidently. Do not include any other text or explanation."

answer_value = {"true": 1, "false": -1, "unknown": 0}

def is_optimization_commit(ci):
    response = client.chat.completions.create(
        model = config.xmcp_model, # model to send to the proxy
        messages = [
            {
                "role": "system",
                "content": system_prompt
            },
            {
                "role": "user",
                "content": str(ci)
            }
        ],
        temperature = 0,
        max_tokens = 10
    )
    answer = response.choices[0].message.content.strip().lower()
    assert answer in answer_value, f"Unexpected answer: {answer}"
    return answer_value[answer]

if __name__ == "__main__":
    repo_path = "../repos/ClickHouse"
    commit_hash = "8ab98c8db0003272fcd572c500fd12e966aac3f8"
    ci = cr.retrieve_commit_info(repo_path, commit_hash)
    print(is_optimization_commit(ci))
