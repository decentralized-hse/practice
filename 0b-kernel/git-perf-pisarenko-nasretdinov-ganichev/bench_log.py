"""
bench_log.py - Benchmark git log / history traversal operations.

Measures:
  - git log --oneline (full history)
  - git rev-list --count HEAD
  - git log -n 100 --oneline (last 100 commits)
  - git log -- <path> (path-filtered log on specific files)
  - git shortlog -sn (author stats)

Requires existing local clones. Set REPO_PATHS or pass paths as CLI args.

Writes results to results/log_results.json
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

PATH_FILTER_TARGETS = {
    "linux": [
        "Makefile",
        "kernel/sched/core.c",
        "drivers/gpu/drm/i915/i915_drv.h",
        "MAINTAINERS",
    ],
    "homebrew-core": [
        "Formula/g/git.rb",
        "Formula/p/python@3.12.rb",
    ],
    "crates.io-index": [
        "se/rd/serde",
        "to/ki/tokio",
    ],
}


def _timed_run(cmd, cwd, capture=True):
    """Run command, return (elapsed_s, stdout_lines_count, success)."""
    print(f"  Running: {' '.join(cmd)}")
    start = time.perf_counter()
    try:
        result = subprocess.run(
            cmd, cwd=cwd, capture_output=capture, text=False, check=True
        )
        elapsed = time.perf_counter() - start
        lines = len(result.stdout.strip().splitlines()) if result.stdout else 0
        return elapsed, lines, True
    except subprocess.CalledProcessError as exc:
        elapsed = time.perf_counter() - start
        print(f"  FAILED ({elapsed:.1f}s): {exc}")
        return elapsed, 0, False
    except FileNotFoundError:
        print(f"  ERROR: git not found or repo path invalid: {cwd}")
        return 0, 0, False


def bench_log(repo_paths=None):
    repo_paths = repo_paths or DEFAULT_REPO_PATHS
    if not repo_paths:
        print("No repository paths configured.")
        print("Usage: python bench_log.py /path/to/repo1 [/path/to/repo2 ...]")
        print("  or set DEFAULT_REPO_PATHS in the script.")
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
        print(f"Benchmarking log operations: {name} ({path})")
        print(f"{'='*60}")

        elapsed, lines, ok = _timed_run(
            ["git", "rev-list", "--count", "HEAD"], cwd=path
        )
        all_results.append({
            "repo": name, "operation": "rev-list --count HEAD",
            "elapsed_s": round(elapsed, 3), "output_lines": lines, "success": ok,
        })
        print(f"  rev-list --count: {elapsed:.3f}s (lines={lines})")

        elapsed, lines, ok = _timed_run(
            ["git", "log", "--oneline"], cwd=path
        )
        all_results.append({
            "repo": name, "operation": "log --oneline (full)",
            "elapsed_s": round(elapsed, 3), "output_lines": lines, "success": ok,
        })
        print(f"  log --oneline (full): {elapsed:.3f}s (lines={lines})")

        elapsed, lines, ok = _timed_run(
            ["git", "log", "-n", "100", "--oneline"], cwd=path
        )
        all_results.append({
            "repo": name, "operation": "log -n 100 --oneline",
            "elapsed_s": round(elapsed, 3), "output_lines": lines, "success": ok,
        })
        print(f"  log -n 100: {elapsed:.3f}s (lines={lines})")

        elapsed, lines, ok = _timed_run(
            ["git", "log", "-n", "1000", "--oneline"], cwd=path
        )
        all_results.append({
            "repo": name, "operation": "log -n 1000 --oneline",
            "elapsed_s": round(elapsed, 3), "output_lines": lines, "success": ok,
        })
        print(f"  log -n 1000: {elapsed:.3f}s (lines={lines})")

        elapsed, lines, ok = _timed_run(
            ["git", "log", "-n", "10000", "--oneline"], cwd=path
        )
        all_results.append({
            "repo": name, "operation": "log -n 10000 --oneline",
            "elapsed_s": round(elapsed, 3), "output_lines": lines, "success": ok,
        })
        print(f"  log -n 10000: {elapsed:.3f}s (lines={lines})")

        elapsed, lines, ok = _timed_run(
            ["git", "shortlog", "-sn", "--no-merges", "HEAD"], cwd=path
        )
        all_results.append({
            "repo": name, "operation": "shortlog -sn --no-merges",
            "elapsed_s": round(elapsed, 3), "output_lines": lines, "success": ok,
        })
        print(f"  shortlog -sn: {elapsed:.3f}s (lines={lines})")

        targets = PATH_FILTER_TARGETS.get(name, [])
        for target_path in targets:
            full_target = os.path.join(path, target_path)
            if not os.path.exists(full_target):
                print(f"  Skipping path-filtered log for {target_path}: not found")
                continue

            elapsed, lines, ok = _timed_run(
                ["git", "log", "--oneline", "--", target_path], cwd=path
            )
            all_results.append({
                "repo": name,
                "operation": f"log --oneline -- {target_path}",
                "elapsed_s": round(elapsed, 3),
                "output_lines": lines,
                "success": ok,
            })
            print(f"  log -- {target_path}: {elapsed:.3f}s (lines={lines})")

    out_path = os.path.join(RESULTS_DIR, "log_results.json")
    with open(out_path, "w") as f:
        json.dump(all_results, f, indent=2)
    print(f"\nResults written to {out_path}")
    return all_results


if __name__ == "__main__":
    custom = None
    if len(sys.argv) > 1:
        custom = []
        for p in sys.argv[1:]:
            p = os.path.abspath(p)
            custom.append({"name": os.path.basename(p), "path": p})
    bench_log(custom)
