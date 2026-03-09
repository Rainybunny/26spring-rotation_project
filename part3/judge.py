import os
import sys
import concurrent.futures
from langchain_core.prompts import ChatPromptTemplate
import utils

def get_mission_list(test_folder) -> list[utils.Mission]:
    mission_list_file = os.path.join(test_folder, "targets.txt")
    mission_list = []
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

    chain = prompt | utils.llm
    try:
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
                print(f"Generating {mission.output_file}...")
                mission.output, mission.opt_reason = future.result()
                with open(mission.output_file, "w") as f:
                    f.write(mission.output)
                log_file = mission.output_file.rsplit('.', 1)[0] + "_or.log"
                with open(log_file, "w") as f:
                    f.write(mission.opt_reason)
            except Exception as exc:
                print(f'{mission.output_file} generated an exception: {exc}')

def judge(mission_list: list[utils.Mission], judge_fn):
    MAX_WORKERS = 10
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        future_to_mission = {
            executor.submit(judge_fn, mission): mission 
            for mission in mission_list
        }
        
        for future in concurrent.futures.as_completed(future_to_mission):
            mission = future_to_mission[future]
            try:
                print(f"Judging {mission.output_file}...")
                mission.result, mission.judge_reason = future.result()
            except Exception as exc:
                print(f'{mission.output_file} generated an exception: {exc}')
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

    mission_list = get_mission_list(test_folder)

    run_optimizer(mission_list, optimizer)

    judge(mission_list, judge_fn)
