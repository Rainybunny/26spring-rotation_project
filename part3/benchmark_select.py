"""
python benchmark_select.py <ratio>
Randomly select and shuffle the (repo_name, commit_hash) pairs in the benchmark folder.
"""

import os
import random
import sys
from pathlib import Path
import utils

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python benchmark_select.py <ratio>")
        sys.exit(1)

    ratio = float(sys.argv[1])
    if not (0 < ratio <= 1):
        print("Ratio must be a float between 0 and 1.")
        sys.exit(1)

    benchmark_path = utils.benchmark_root
    all_pairs = []
    
    for repo_name in os.listdir(benchmark_path):
        repo_path = os.path.join(benchmark_path, repo_name)
        commit_path = os.path.join(repo_path, "modified_file")
        if not os.path.isdir(commit_path):
            continue
        for commit_hash in os.listdir(commit_path):
            all_pairs.append((repo_name, commit_hash))
    
    random.shuffle(all_pairs)
    selected_count = int(len(all_pairs) * ratio)
    selected_pairs = all_pairs[:selected_count]

    print(f"{selected_count} pairs selected.", file=sys.stderr)
    for repo_name, commit_hash in selected_pairs:
        print(f"{repo_name} {commit_hash}")
