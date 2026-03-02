"""
给定代码库和一个提交哈希值, 返回该提交的提交信息. 我们总用 None 表示失败的获取, 用空串表示确实空的内容.
CommitInfo:
    - commit_hash: 提交哈希值
    - commit_message: 提交信息
    - changes: list of FileDelta, 每个FileDelta包含:
        > path: 文件路径
        > status: 文件状态 (A=added, M=modified, D=deleted, R=renamed)
        > diff: 该文件在该提交中的diff内容
        > content_before: 该文件在该提交之前的内容 (对于新增文件, 为空串)
        > content_after: 该文件在该提交之后的内容 (对于删除文件, 为空串)
"""

import os
import subprocess
from subprocess import CalledProcessError, PIPE
import sys

class FileDelta:
    def __init__(self, path, status):
        self.path = path
        self.status = status
        self.diff = None
        self.content_before = None
        self.content_after = None

class CommitInfo:
    def __init__(self, commit_hash):
        self.commit_hash = commit_hash
        self.commit_message = None
        self.changes = None
    
    @property
    def changed_files(self):
        if self.changes is None:
            return None
        return [c.path for c in self.changes]
    
    @property
    def all_diffs(self):
        if self.changes is None:
            return None
        return "\n".join(c.diff for c in self.changes if c.diff is not None)

    def __str__(self):
        res = f"Commit: {self.commit_hash}\nMessage: {self.commit_message}\n"
        if self.changes is not None:
            for c in self.changes:
                res += f"File: {c.path} Status: {c.status}\n"
                res += f"Diff:\n{c.diff}\n"
                res += f"Content Before:\n{c.content_before}\n"
                res += f"Content After:\n{c.content_after}\n"
        return res

def download_repo(repo_url, dest_path):
    try:
        print(f"Cloning repository from {repo_url} to {dest_path}...")
        proc = subprocess.run(['git', 'clone', repo_url, dest_path], check=True, stdout=PIPE, stderr=PIPE, text=True)
        return True
    except CalledProcessError as e:
        print(f"Error cloning repository: {e.stderr}", file=sys.stderr)
        return False

def retrieve_commit_info(repo_path, commit_hash, repo_url=None):
    if repo_url and not os.path.exists(repo_path):
        if not download_repo(repo_url, repo_path):
            return None

    ci = CommitInfo(commit_hash)

    # commit message
    try:
        proc = subprocess.run([
            'git', '-C', repo_path, 'log', '-1', '--pretty=%B', commit_hash
        ], check=True, stdout=PIPE, stderr=PIPE, text=True)
        ci.commit_message = proc.stdout.strip()
    except CalledProcessError:
        return ci

    # changed files and statuses
    try:
        proc = subprocess.run([
            'git', '-C', repo_path, 'diff-tree', '--no-commit-id', '--name-status', '-r', commit_hash
        ], check=True, stdout=PIPE, stderr=PIPE, text=True)
        lines = [l for l in proc.stdout.splitlines() if l.strip()]
    except CalledProcessError:
        return ci

    ci.changes = []

    for line in lines:
        # lines look like: "M\tpath" or "A\tpath" or "R100\told\tnew"
        parts = line.split('\t')
        status = parts[0]
        if status.startswith('R'):
            # rename: parts[1]=old, parts[2]=new
            old_path = parts[1]
            new_path = parts[2]
            path = new_path
        else:
            old_path = None
            new_path = None
            path = parts[1]

        fd = FileDelta(path, status)

        # get diff for this file between parent and this commit
        try:
            diff_proc = subprocess.run([
                'git', '-C', repo_path, 'diff', '-U0', f'{commit_hash}^', commit_hash, '--', path
            ], check=True, stdout=PIPE, stderr=PIPE, text=True)
            fd.diff = diff_proc.stdout
        except CalledProcessError:
            fd.diff = None

        # content before (use parent commit). For added files, there is no before content.
        if status.startswith('A'):
            fd.content_before = ""
        else:
            # choose correct path for before (handle rename)
            path_before = old_path if old_path else path
            try:
                cb = subprocess.run([
                    'git', '-C', repo_path, 'show', f'{commit_hash}^:{path_before}'
                ], check=True, stdout=PIPE, stderr=PIPE, text=True)
                fd.content_before = cb.stdout
            except CalledProcessError:
                fd.content_before = None

        # content after (may be missing for deleted files)
        if status.startswith('D'):
            fd.content_after = ""
        else:
            path_after = new_path if new_path else path
            try:
                ca = subprocess.run([
                    'git', '-C', repo_path, 'show', f'{commit_hash}:{path_after}'
                ], check=True, stdout=PIPE, stderr=PIPE, text=True)
                fd.content_after = ca.stdout
            except CalledProcessError:
                fd.content_after = None

        ci.changes.append(fd)

    return ci


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python commit_retrieve.py <repo_path> <commit_hash> [repo_url]", file=sys.stderr)
        sys.exit(1)

    repo_path = sys.argv[1]
    commit_hash = sys.argv[2]
    repo_url = sys.argv[3] if len(sys.argv) > 3 else None

    info = retrieve_commit_info(repo_path, commit_hash, repo_url)
    if info is None:
        print("Failed to retrieve commit info", file=sys.stderr)
        sys.exit(1)

    print(f"Commit: {info.commit_hash}")
    print(f"Message: {info.commit_message}")
    for change in info.changes:
        print(f"File: {change.path} Status: {change.status}")
        print(f"Diff:\n{change.diff}")
        print(f"Content Before:\n{change.content_before}")
        print(f"Content After:\n{change.content_after}")

