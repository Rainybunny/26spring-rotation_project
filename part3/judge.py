import os
import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent.absolute()))
import config
from langchain_openai import ChatOpenAI
from langchain_core.prompts import ChatPromptTemplate

project_root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
benchmark_root = os.path.join(project_root, "benchmark")

def get_target_list(folder):
    target_list_file = os.path.join(folder, "targets.txt")
    target_list = []
    with open(target_list_file, "r") as f:
        for line in f:
            repo_name, commit_hash = line.strip().split()
            commit_folder = os.path.join(benchmark_root,
                                         repo_name, "modified_file", commit_hash)
            before_file = None
            after_file = None
            for file in os.listdir(commit_folder):
                if file.startswith("before."):
                    before_file = os.path.join(commit_folder, file)
                elif file.startswith("after."):
                    after_file = os.path.join(commit_folder, file)
            assert before_file is not None and after_file is not None, \
                f"Missing before/after file in {commit_folder}"
            trunked_commit_hash = commit_hash[:3] + commit_hash[-3:]
            output_name = f"{repo_name}_{trunked_commit_hash}_output.{after_file.split('.')[-1]}"
            output_file = os.path.join(folder, output_name)
            target_list.append((before_file, after_file, output_file))
    return target_list


def code_regularize(file):
    """
    Do C/CPP code regularization
    """
    pass


def exact_match(origin, target, output):
    target_r = code_regularize(target)
    output_r = code_regularize(output)
    if target_r != output_r:
        return False, "Just not match."
    else:
        return True, "Just match."


def llm_judge(origin, target, output):
    llm = ChatOpenAI(
        openai_api_key=config.xmcp_api_key,
        openai_api_base=config.xmcp_base_url,
        model_name=config.xmcp_model,
        temperature=0.0
    )

    system_prompt = (
        "You are an expert code judge. Your task is to determine if the provided C/C++ optimized code is a valid optimization of the original code. "
        "The optimized code should improve runtime efficiency or reduce resource consumption compared to the original code, while maintaining correctness.\n"
        "You will be given two pieces of information: the original code and the output from an optimization process.\n"
        "1. Check if the optimized code is a FULL, VALID source file, not just a snippet. If it is a snippet or incomplete, it is INVALID.\n"
        "2. Analyze the optimized code for correctness and performance.\n"
        "3. Provide your reasoning.\n"
        "4. End your response with 'VERDICT: TRUE' if it is a valid optimization, or 'VERDICT: FALSE' if it is invalid.\n"
    )

    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "Original Code:\n```\n{origin}\n```\n\nOptimized Code:\n```\n{output}\n```")
    ])

    chain = prompt | llm
    try:
        response = chain.invoke({"origin": origin, "output": output})
        content = response.content
        
        reason = content
        verdict = False
        
        # Check for the verdict line
        content_lines = content.strip().split('\n')
        last_line = content_lines[-1].strip()
        
        if "VERDICT: TRUE" in last_line.upper():
            verdict = True
            reason = "\n".join(content_lines[:-1]).strip()
        elif "VERDICT: FALSE" in last_line.upper():
            verdict = False
            reason = "\n".join(content_lines[:-1]).strip()
        else:
            # Fallback if format is not strictly followed but contains keywords
            if "VERDICT: TRUE" in content.upper():
                verdict = True
            elif "VERDICT: FALSE" in content.upper():
                verdict = False
            # reason is the whole content in this fallback case
            
        return verdict, reason

    except Exception as e:
        print(f"Error during LLM judging: {e}")
        return False, f"Error: {str(e)}"


def run_optimizer(target_list, optimizer):
    for before_file, _, output_file in target_list:
        print(f"Generating {output_file}...")
        origin = open(before_file, "r").read()
        optimized, reason = optimizer(origin)
        with open(output_file, "w") as f:
            f.write(optimized)
        log_file = output_file.rsplit('.', 1)[0] + "_or.log"
        with open(log_file, "w") as f:
            f.write(reason)

def judge(folder, judge_fn):
    target_list = get_target_list(folder)
    results = []
    total = len(target_list)
    accepted = 0
    for before_file, after_file, output_file in target_list:
        print(f"Judging {output_file}...")
        origin = open(before_file, "r").read()
        target = open(after_file, "r").read()
        output = open(output_file, "r").read()
        result, reason = judge_fn(origin, target, output)
        if result:
            accepted += 1
            results.append('A')
        else:
            results.append('R')
        log_file = output_file.rsplit('.', 1)[0] + "_jr.log"
        with open(log_file, "w") as f:
            f.write(reason)
    print(f"{accepted}/{total} accepted\nstatus: {''.join(results)}")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python judge.py <test_folder> <optimizer_py> <judge_fn_name>")
        sys.exit(1)
    test_folder = sys.argv[1]
    optimizer_py = sys.argv[2]
    judge_fn_name = sys.argv[3]

    sys.path.append(os.path.dirname(optimizer_py))
    optimizer_module = __import__(os.path.basename(optimizer_py).split('.')[0])
    optimizer = optimizer_module.optimize
    judge_fn = globals()[judge_fn_name]

    target_list = get_target_list(test_folder)

    run_optimizer(target_list, optimizer)

    judge(test_folder, judge_fn)
