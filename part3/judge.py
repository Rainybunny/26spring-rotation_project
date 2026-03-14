import os
import sys
import concurrent.futures
from time import time
from langchain_core.prompts import ChatPromptTemplate
import utils

def get_mission_list(test_folder, opt_id) -> list[utils.Mission]:
    mission_list_file = os.path.join(test_folder, "targets.txt")
    mission_list = []
    test_folder = os.path.join(test_folder, opt_id)
    with open(mission_list_file, "r") as f:
        for line in f:
            repo_name, commit_hash = line.strip().split()
            mission_list.append(utils.Mission(repo_name, commit_hash, test_folder))
    return mission_list

def llm_judge(mission: utils.Mission) -> tuple[bool, str]:
    system_prompt = (
        "You are an expert code judge. Your task is to determine if the provided C/C++ optimized function is a valid optimization of the original function within the context of the full source code.\n"
        "The optimized function should improve runtime efficiency or reduce resource consumption compared to the original implementation, while maintaining correctness.\n"
        "You will be given the full original source code and the optimized function implementation.\n"
        "1. Check if the optimized code is a valid replacement for the specific function. It should be just the function implementation.\n"
        "2. Analyze the optimized function for correctness and performance.\n"
        "3. Provide your reasoning.\n"
        "4. End your response with 'VERDICT: TRUE' if it is a valid optimization, or 'VERDICT: FALSE' if it is invalid.\n"
    )

    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{result_content}")
    ])

    chain = prompt | utils.judge_llm
    # chain = prompt | utils.llm 
    try:
        # Add a timeout to prevent hanging indefinitely
        response = chain.invoke({"result_content": mission.show_result()})
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

def llm_judge_strict(mission: utils.Mission) -> tuple[bool, str]:
    system_prompt = (
        "You are an expert code judge. Your task is to determine if the provided C/C++ optimized function is a valid optimization "
        "of the original function within the context of the full source code, AND if it follows the same optimization strategy "
        "as the reference solution.\n\n"
        "You will be provided with:\n"
        "1. Full original source code.\n"
        "2. The PROPOSED optimized function implementation.\n"
        "3. The REFERENCE optimized function implementation (Ground Truth).\n\n"
        "Evaluation Criteria:\n"
        "1. VALIDITY: The proposed code must be a valid replacement for the original function, maintaining correctness.\n"
        "2. PERFORMANCE: It should improve runtime efficiency or reduce resource consumption.\n"
        "3. STRATEGY MATCH: The optimization idea/strategy in the PROPOSED solution MUST match the detailed idea in the REFERENCE solution. "
        "If the reference uses a specific algorithmic change, data structure, or low-level trick, the proposal must use the same. "
        "Reject if the strategies differ significantly, even if the proposal is valid on its own.\n\n"
        "Output Format:\n"
        "1. Analysis of the proposed optimization correctness and performance.\n"
        "2. Comparison of the optimization strategy vs the reference strategy.\n"
        "3. Final Verdict line: 'VERDICT: TRUE' or 'VERDICT: FALSE'.\n"
    )

    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{result_content}")
    ])

    chain = prompt | utils.judge_llm
    # chain = prompt | utils.llm 
    try:
        # Add a timeout to prevent hanging indefinitely
        result_content = mission.show_result()
        result_content += f"\n\nReference Optimized Function (Ground Truth):\n```{mission.lang}\n{mission.target_func}\n```\n"
        response = chain.invoke({"result_content": result_content})
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

def run_optimizer(mission_list: list[utils.Mission], optimizer):
    # Determine max workers, default to 10 if not specified (or use user token limit)
    MAX_WORKERS = 10
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        # Map futures to missions so we know which mission corresponds to which result
        future_to_mission = {
            executor.submit(optimizer, mission): mission 
            for mission in mission_list
        }
        
        for future in concurrent.futures.as_completed(future_to_mission):
            mission = future_to_mission[future]
            try:
                print(f"Generating {mission.output_file}...", flush=True)
                mission.output, mission.opt_reason = future.result()
                with open(mission.output_file, "w") as f:
                    f.write(mission.output)
                log_file = mission.output_file.rsplit('.', 1)[0] + "_or.log"
                with open(log_file, "w") as f:
                    f.write(mission.opt_reason)
            except Exception as exc:
                print(f'{mission.output_file} generated an exception: {exc}', flush=True)

judge_fn = llm_judge
# judge_fn = llm_judge_strict

def judge(mission_list: list[utils.Mission]):
    MAX_WORKERS = 10
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        future_to_mission = {
            executor.submit(judge_fn, mission): mission 
            for mission in mission_list
        }
        
        for future in concurrent.futures.as_completed(future_to_mission):
            mission = future_to_mission[future]
            try:
                print(f"Judging {mission.output_file}...", flush=True)
                mission.result, mission.judge_reason = future.result()
            except Exception as exc:
                print(f'{mission.output_file} generated an exception: {exc}', flush=True)
                mission.result = False
                mission.judge_reason = f"Exception: {exc}"
            
            # Write judge reason log
            log_file = mission.output_file.rsplit('.', 1)[0] + "_jr.log"
            with open(log_file, "w") as f:
                f.write(str(mission.judge_reason))

    # Calculate final stats in order
    results = []
    total = len(mission_list)
    accepted = 0
    for mission in mission_list:
        if mission.result:
            accepted += 1
            results.append('A')
        else:
            results.append('R')
    print(f"{accepted}/{total} accepted\nstatus: {''.join(results)}")
    print("== details ==")
    for mission in mission_list:
        print(f"> {mission.repo_name}/{mission.commit_hash}: {'A' if mission.result else 'R'}")

optimizer_id = {
    "dio": "optimizer_direct_io.py",
    "rag": "optimizer_rag.py",
    "ccs": "optimizer_ccskill.py"
}

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python judge.py <test_name> <optimizers...>", file=sys.stderr)
        sys.exit(1)
    test_folder = os.path.join(utils.test_root, sys.argv[1])

    opt_num = len(sys.argv) - 2
    print(f"{opt_num} optimizers specified: {sys.argv[2:]}\n", file=sys.stderr)
    for opt_id in sys.argv[2:]:
        if opt_id not in optimizer_id:
            print(f"Unknown optimizer '{opt_id}'.", file=sys.stderr)
            sys.exit(1)

    for opt_id in sys.argv[2:]:
        opt_name = optimizer_id[opt_id]
        print(f"== Testing optimizer: {opt_id} ({opt_name}) ==", file=sys.stderr)
        # flush stdout and redirect to log file
        os.makedirs(os.path.join(test_folder, opt_id), exist_ok=True)
        sys.stdout.flush()
        log_file = os.path.join(test_folder, opt_id, "_results.log")
        sys.stdout = open(log_file, "w")

        opt_py = os.path.join(os.path.dirname(os.path.abspath(__file__)), opt_name)
        opt_mod = __import__(os.path.basename(opt_py).split('.')[0])
        optimizer = opt_mod.optimize
        mission_list = get_mission_list(test_folder, opt_id)
        print(f"{len(mission_list)} missions loaded.", file=sys.stderr)

        print("Optimizing...", end='', file=sys.stderr, flush=True)
        t_start = time()
        run_optimizer(mission_list, optimizer)
        t_end = time()
        print(f"\tdone in {t_end - t_start:.2f} seconds", file=sys.stderr)
        print("Judging...", end='', file=sys.stderr, flush=True)
        t_start = time()
        judge(mission_list)
        t_end = time()
        print(f"\tdone in {t_end - t_start:.2f} seconds", file=sys.stderr, flush=True)
