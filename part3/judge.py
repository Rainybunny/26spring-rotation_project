import os
import sys
import concurrent.futures
import re
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

def regularize_c_code(code: str) -> str:
    if code is None:
        return ""

    text = code.strip()

    # Accept markdown-wrapped code and keep only the last fenced block.
    fence_matches = list(re.finditer(r"```(?:c|cpp|cc|cxx|h|hpp)?\s*\n(.*?)```", text, re.DOTALL | re.IGNORECASE))
    if fence_matches:
        text = fence_matches[-1].group(1)

    out = []
    i = 0
    n = len(text)

    in_single_quote = False
    in_double_quote = False

    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        # Preserve content in string and char literals to avoid corrupting code.
        if in_single_quote:
            out.append(ch)
            if ch == "\\" and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if ch == "'":
                in_single_quote = False
            i += 1
            continue

        if in_double_quote:
            out.append(ch)
            if ch == "\\" and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if ch == '"':
                in_double_quote = False
            i += 1
            continue

        if ch == "'":
            in_single_quote = True
            out.append(ch)
            i += 1
            continue

        if ch == '"':
            in_double_quote = True
            out.append(ch)
            i += 1
            continue

        # Remove C/C++ comments outside of strings/chars.
        if ch == "/" and nxt == "/":
            i += 2
            while i < n and text[i] != "\n":
                i += 1
            continue

        if ch == "/" and nxt == "*":
            i += 2
            while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i += 2 if i + 1 < n else 0
            continue

        out.append(ch)
        i += 1

    text = "".join(out)

    # Normalize all whitespace differences to a single representation.
    text = re.sub(r"\s+", "", text)
    return text

def exact_match(mission: utils.Mission) -> tuple[bool, str]:
    out_norm = regularize_c_code(mission.output)
    target_norm = regularize_c_code(mission.target_func)

    if out_norm == target_norm:
        return True, "Exact match after C/C++ regularization."
    return False, "Mismatch after C/C++ regularization."

def llm_judge_strict(mission: utils.Mission) -> tuple[bool, str]:
    system_prompt = (
        "You are an expert C/C++ optimization judge.\n"
        "You will receive exactly 3 labeled snippets: A, B, and C.\n"
        "Interpret labels strictly:\n"
        "- A = ORIGINAL_FULL_SOURCE (pre-optimization full code).\n"
        "- B = PROPOSED_OPTIMIZED_FUNCTION (candidate replacement function).\n"
        "- C = REFERENCE_OPTIMIZED_FUNCTION (ground-truth optimized replacement function).\n"
        "Critical anti-confusion rule: B and C are both optimized-function candidates; neither is the original full code.\n"
        "Never claim C is the original function unless the content itself clearly proves that.\n\n"
        "Evaluation criteria:\n"
        "1. VALIDITY: B must be a valid replacement for the target function in A, preserving behavior.\n"
        "2. PERFORMANCE: B should plausibly improve performance/resource usage over the original implementation in A.\n"
        "3. STRATEGY MATCH: B must use substantially the same optimization idea as C.\n"
        "   Reject if B is valid but uses a materially different optimization strategy.\n\n"
        "Output format:\n"
        "1. Short analysis for VALIDITY and PERFORMANCE.\n"
        "2. Strategy comparison: B vs C.\n"
        "3. Final line exactly: VERDICT: TRUE or VERDICT: FALSE\n"
    )

    prompt = ChatPromptTemplate.from_messages([
        ("system", system_prompt),
        ("user", "{result_content}")
    ])

    chain = prompt | utils.judge_llm
    # chain = prompt | utils.llm

    # let exception propagate to trigger retry in case of LLM issues,
    # instead of silently returning False

    # Add a timeout to prevent hanging indefinitely
    result_content = (
        "SNIPPET A [ORIGINAL_FULL_SOURCE]:\n"
        f"```{mission.lang}\n{mission.origin}\n```\n\n"
        "SNIPPET B [PROPOSED_OPTIMIZED_FUNCTION]:\n"
        "(This is the candidate optimized replacement function, not the original full code.)\n"
        f"```{mission.lang}\n{mission.output}\n```\n\n"
        "SNIPPET C [REFERENCE_OPTIMIZED_FUNCTION]:\n"
        "(This is the ground-truth optimized replacement function, not the original full code.)\n"
        f"```{mission.lang}\n{mission.target_func}\n```\n\n"
        "Reminder: Use snippet labels A/B/C exactly as defined above.\n"
    )

    # Save the fully rendered prompt payload sent to judge_llm.
    rendered_messages = prompt.format_messages(result_content=result_content)
    mission.judge_prompt = "\n\n".join(
        f"[{msg.type}]\n{msg.content}" for msg in rendered_messages
    )

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

def run_optimizer(mission_list: list[utils.Mission], optimizer):
    if optimizer is None:
        print(" Oh, just loading for optimizer=None...",
              end='', file=sys.stderr, flush=True)
        for mission in mission_list:
            print(f"Loading {mission.output_file}...", flush=True)
            with open(mission.output_file, "r") as f:
                mission.output = f.read()

            log_file = mission.output_file.rsplit('.', 1)[0] + "_or.log"
            with open(log_file, "r") as f:
                mission.opt_reason = f.read()

            prompt_log_file = mission.output_file.rsplit('.', 1)[0] + "_op.log"
            if os.path.exists(prompt_log_file):
                with open(prompt_log_file, "r") as f:
                    mission.opt_prompt = f.read()
            else:
                mission.opt_prompt = None
        return

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
            ok = False
            while not ok:
                try:
                    print(f"Generating {mission.output_file}...", flush=True)
                    mission.output, mission.opt_reason = future.result()
                    with open(mission.output_file, "w") as f:
                        f.write(mission.output)
                    log_file = mission.output_file.rsplit('.', 1)[0] + "_or.log"
                    with open(log_file, "w") as f:
                        f.write(mission.opt_reason)
                    prompt_log_file = mission.output_file.rsplit('.', 1)[0] + "_op.log"
                    with open(prompt_log_file, "w") as f:
                        f.write(str(mission.opt_prompt) if mission.opt_prompt is not None else "")
                    ok = True
                except Exception as exc:
                    print(f'{mission.output_file} generated an exception: {exc}', flush=True)
                    print(f'Retrying this...', flush=True)

judge_fn = llm_judge_strict
# judge_fn = exact_match

def judge(mission_list: list[utils.Mission]):
    MAX_WORKERS = 10 # no parallelization
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        future_to_mission = {
            executor.submit(judge_fn, mission): mission 
            for mission in mission_list
        }
        
        for future in concurrent.futures.as_completed(future_to_mission):
            mission = future_to_mission[future]
            ok = False
            while not ok:
                try:
                    print(f"Judging {mission.output_file}...", flush=True)
                    mission.result, mission.judge_reason = future.result()
                    ok = True
                except Exception as exc:
                    print(f'{mission.output_file} generated an exception: {exc}', flush=True)
                    mission.result = False
                    mission.judge_reason = f"Exception: {exc}"
                    print(f'Retrying this...', flush=True)
            
            # Write judge reason log
            log_file = mission.output_file.rsplit('.', 1)[0] + "_jr.log"
            with open(log_file, "w") as f:
                f.write(str(mission.judge_reason))

            # Write judge prompt log (if available from llm_judge_strict).
            prompt_log_file = mission.output_file.rsplit('.', 1)[0] + "_jp.log"
            with open(prompt_log_file, "w") as f:
                f.write(str(getattr(mission, "judge_prompt", "")))

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
    "rgn": "optimizer_rag_noex.py",
    "rgn2": "optimizer_rag_noex.py", # same, run again for more results
    "ccs": "optimizer_ccskill.py",
}

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python judge.py <test_name> [-jo] <optimizers...>", file=sys.stderr)
        sys.exit(1)
    test_folder = os.path.join(utils.test_root, sys.argv[1])

    cli_args = sys.argv[2:]
    judge_only = "-jo" in cli_args
    optimizer_args = [arg for arg in cli_args if arg != "-jo"]
    if not optimizer_args:
        print("No optimizer specified.", file=sys.stderr)
        print("Usage: python judge.py <test_name> [-jo] <optimizers...>", file=sys.stderr)
        sys.exit(1)

    opt_num = len(optimizer_args)
    print(f"{opt_num} optimizers specified: {optimizer_args} (judge_only={judge_only})\n", file=sys.stderr)
    for opt_id in optimizer_args:
        if opt_id not in optimizer_id:
            print(f"Unknown optimizer '{opt_id}'.", file=sys.stderr)
            sys.exit(1)

    for opt_id in optimizer_args:
        opt_name = optimizer_id[opt_id]
        print(f"== Testing optimizer: {opt_id} ({opt_name}) ==", file=sys.stderr)
        # flush stdout and redirect to log file
        os.makedirs(os.path.join(test_folder, opt_id), exist_ok=True)
        sys.stdout.flush()
        log_file = os.path.join(test_folder, opt_id, "_results.log")
        sys.stdout = open(log_file, "w")

        if judge_only:
            optimizer = None
        else:
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
