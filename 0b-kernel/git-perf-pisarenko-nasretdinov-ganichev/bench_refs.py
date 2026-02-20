"""
bench_refs.py - Benchmark reference enumeration operations.

Measures:
  - git tag -l (list all tags)
  - git branch -a (list all branches)
  - git for-each-ref (list all refs)
  - git ls-remote <url> (remote ref listing - requires network)

Requires existing local clones. Set REPO_PATHS or pass paths as CLI args.

Writes results to results/refs_results.json
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

REMOTE_URLS = {
    "linux": "https://github.com/torvalds/linux.git",
    "homebrew-core": "https://github.com/Homebrew/homebrew-core.git",
    "crates.io-index": "https://github.com/rust-lang/crates.io-index.git",
}


def _timed_run(cmd, cwd=None):
    """Run command, return (elapsed_s, output_lines, success)."""
    print(f"  Running: {' '.join(cmd)}")
    start = time.perf_counter()
    try:
        result = subprocess.run(
            cmd, cwd=cwd, capture_output=True, text=False, check=True
        )
        elapsed = time.perf_counter() - start
        lines = len(result.stdout.strip().splitlines()) if result.stdout else 0
        return elapsed, lines, True
    except subprocess.CalledProcessError as exc:
        elapsed = time.perf_counter() - start
        print(f"  FAILED ({elapsed:.1f}s): {exc}")
        return elapsed, 0, False
    except FileNotFoundError:
        print(f"  ERROR: git not found")
        return 0, 0, False


def bench_refs(repo_paths=None):
    repo_paths = repo_paths or DEFAULT_REPO_PATHS
    if not repo_paths:
        print("No repository paths configured.")
        print("Usage: python bench_refs.py /path/to/repo1 [/path/to/repo2 ...]")
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
        print(f"Benchmarking refs: {name} ({path})")
        print(f"{'='*60}")

        elapsed, lines, ok = _timed_run(["git", "tag", "-l"], cwd=path)
        all_results.append({
            "repo": name, "operation": "tag -l",
            "elapsed_s": round(elapsed, 3), "ref_count": lines, "success": ok,
        })
        print(f"  tag -l: {elapsed:.3f}s ({lines} tags)")

        elapsed, lines, ok = _timed_run(["git", "branch", "-a"], cwd=path)
        all_results.append({
            "repo": name, "operation": "branch -a",
            "elapsed_s": round(elapsed, 3), "ref_count": lines, "success": ok,
        })
        print(f"  branch -a: {elapsed:.3f}s ({lines} branches)")

        elapsed, lines, ok = _timed_run(["git", "for-each-ref"], cwd=path)
        all_results.append({
            "repo": name, "operation": "for-each-ref",
            "elapsed_s": round(elapsed, 3), "ref_count": lines, "success": ok,
        })
        print(f"  for-each-ref: {elapsed:.3f}s ({lines} refs)")

        elapsed, lines, ok = _timed_run(
            ["git", "for-each-ref", "refs/tags"], cwd=path
        )
        all_results.append({
            "repo": name, "operation": "for-each-ref refs/tags",
            "elapsed_s": round(elapsed, 3), "ref_count": lines, "success": ok,
        })
        print(f"  for-each-ref refs/tags: {elapsed:.3f}s ({lines} tag refs)")

        url = REMOTE_URLS.get(name)
        if url:
            elapsed, lines, ok = _timed_run(["git", "ls-remote", url])
            all_results.append({
                "repo": name, "operation": f"ls-remote {url}",
                "elapsed_s": round(elapsed, 3), "ref_count": lines, "success": ok,
            })
            print(f"  ls-remote: {elapsed:.3f}s ({lines} remote refs)")

    out_path = os.path.join(RESULTS_DIR, "refs_results.json")
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
    bench_refs(custom)
