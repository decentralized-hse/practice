"""
bench_blame.py - Benchmark git blame on files of various sizes.

Measures wall-clock time of `git blame <file>` for files with
different line counts and history depths.

Requires existing local clones. Set REPO_PATHS or pass paths as CLI args.

Writes results to results/blame_results.json
"""

import json
import os
import subprocess
import sys
import time

RESULTS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results")

DEFAULT_REPO_PATHS = [
    {"name": "linux", "path": "/home/grigoriypisar/HW/repos/linux"},
    {"name": "homebrew-core", "path": "/home/grigoriypisar/HW/repos/homebrew-core"},
    {"name": "crates.io-index", "path": "/home/grigoriypisar/HW/repos/crates.io-index"},
]

BLAME_TARGETS = {
    "linux": [
        "Makefile",
        "README",
        "CREDITS",
        "MAINTAINERS",
        "kernel/fork.c",
        "kernel/sched/core.c",
        "mm/memory.c",
        "fs/read_write.c",
        "init/main.c",
        "include/linux/fs.h",
    ],
    "homebrew-core": [
        "Formula/g/git.rb",
        "Formula/w/wget.rb",
        "Formula/p/python@3.12.rb",
    ],
    "crates.io-index": [
        "se/rd/serde",
        "to/ki/tokio",
        "ra/nd/rand",
    ],
}


def _count_file_lines(filepath):
    """Count lines in a file."""
    try:
        with open(filepath, "r", errors="replace") as f:
            return sum(1 for _ in f)
    except Exception:
        return 0


def _timed_blame(filepath, cwd):
    """Run git blame and return (elapsed_s, line_count, success)."""
    cmd = ["git", "blame", "--line-porcelain", filepath]
    print(f"  Running: git blame {filepath}")
    start = time.perf_counter()
    try:
        result = subprocess.run(
            cmd, cwd=cwd, capture_output=True, text=False, check=True
        )
        elapsed = time.perf_counter() - start
        author_lines = sum(
            1 for line in result.stdout.splitlines()
            if line.startswith(b"author ")
        )
        return elapsed, author_lines, True
    except subprocess.CalledProcessError as exc:
        elapsed = time.perf_counter() - start
        stderr_snippet = exc.stderr[:200] if exc.stderr else str(exc)
        print(f"  FAILED ({elapsed:.1f}s): {stderr_snippet}")
        return elapsed, 0, False
    except FileNotFoundError:
        print(f"  ERROR: git not found or repo path invalid: {cwd}")
        return 0, 0, False


def bench_blame(repo_paths=None):
    repo_paths = repo_paths or DEFAULT_REPO_PATHS
    if not repo_paths:
        print("No repository paths configured.")
        print("Usage: python bench_blame.py /path/to/repo1 [/path/to/repo2 ...]")
        return []

    os.makedirs(RESULTS_DIR, exist_ok=True)
    all_results = []

    for repo in repo_paths:
        name = repo["name"]
        path = repo["path"]

        if not os.path.isdir(path):
            print(f"Skipping {name}: {path} not found")
            continue

        print(f"\n{'='*60}")
        print(f"Benchmarking blame: {name} ({path})")
        print(f"{'='*60}")

        targets = BLAME_TARGETS.get(name, [])
        if not targets:
            print("  No predefined targets; auto-discovering files...")
            targets = _auto_discover_files(path)

        for target in targets:
            full_path = os.path.join(path, target)
            if not os.path.isfile(full_path):
                print(f"  Skipping {target}: not found")
                continue

            file_lines = _count_file_lines(full_path)
            elapsed, blamed_lines, ok = _timed_blame(target, cwd=path)

            result = {
                "repo": name,
                "file": target,
                "file_lines": file_lines,
                "blamed_lines": blamed_lines,
                "elapsed_s": round(elapsed, 3),
                "success": ok,
            }
            all_results.append(result)
            print(f"  {target}: {elapsed:.3f}s | file_lines={file_lines} | blamed={blamed_lines}")

    out_path = os.path.join(RESULTS_DIR, "blame_results.json")
    with open(out_path, "w") as f:
        json.dump(all_results, f, indent=2)
    print(f"\nResults written to {out_path}")
    return all_results


def _auto_discover_files(repo_path, max_files=10):
    """Find some representative files in the repo to blame."""
    discovered = []
    try:
        result = subprocess.run(
            ["git", "ls-files"],
            cwd=repo_path, capture_output=True, text=True, check=True
        )
        all_files = result.stdout.strip().splitlines()
    except Exception:
        return discovered

    sized = []
    for f in all_files[:5000]:
        full = os.path.join(repo_path, f)
        if os.path.isfile(full):
            lines = _count_file_lines(full)
            if lines > 0:
                sized.append((f, lines))

    sized.sort(key=lambda x: x[1])

    if len(sized) == 0:
        return discovered

    step = max(1, len(sized) // max_files)
    for i in range(0, len(sized), step):
        discovered.append(sized[i][0])
        if len(discovered) >= max_files:
            break

    return discovered


if __name__ == "__main__":
    custom = None
    if len(sys.argv) > 1:
        custom = []
        for p in sys.argv[1:]:
            p = os.path.abspath(p)
            custom.append({"name": os.path.basename(p), "path": p})
    bench_blame(custom)
