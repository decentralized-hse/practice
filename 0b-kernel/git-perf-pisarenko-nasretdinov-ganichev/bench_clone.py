"""
bench_clone.py - Benchmark git clone with different strategies.

Measures:
  - Full clone (default)
  - Shallow clone (--depth=1)
  - Partial / blobless clone (--filter=blob:none)

Writes results to results/clone_results.json
"""

import json
import os
import shutil
import subprocess
import sys
import time

RESULTS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results")

DEFAULT_REPOS = [
    {"name": "linux",           "url": "https://github.com/torvalds/linux.git"},
    {"name": "homebrew-core",   "url": "https://github.com/Homebrew/homebrew-core.git"},
    {"name": "crates.io-index", "url": "https://github.com/rust-lang/crates.io-index.git"},
]

CLONE_STRATEGIES = [
    {"label": "full",    "extra_args": []},
    {"label": "shallow", "extra_args": ["--depth=1"]},
    {"label": "partial", "extra_args": ["--filter=blob:none"]},
]

WORK_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_clone_workdir")


def _run_clone(url, dest, extra_args):
    """Run git clone and return (elapsed_seconds, success)."""

    cmd = ["git", "clone"] + extra_args + [url, dest]
    print(f"  Running: {' '.join(cmd)}")
    start = time.perf_counter()
    try:
        subprocess.run(cmd, check=True, capture_output=False)
        elapsed = time.perf_counter() - start
        return elapsed, True
    except subprocess.CalledProcessError as exc:
        elapsed = time.perf_counter() - start
        print(f"  FAILED ({elapsed:.1f}s): {exc.stderr[:300] if exc.stderr else exc}")
        return elapsed, False


def _dir_size_mb(path):
    """Recursively compute directory size in MB."""

    total = 0
    for dirpath, _dirnames, filenames in os.walk(path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            if os.path.isfile(fp):
                total += os.path.getsize(fp)
    return round(total / (1024 * 1024), 2)


def bench_clone(repos=None):
    repos = repos or DEFAULT_REPOS
    os.makedirs(RESULTS_DIR, exist_ok=True)
    os.makedirs(WORK_DIR, exist_ok=True)

    all_results = []

    for repo in repos:
        name = repo["name"]
        url = repo["url"]
        print(f"\n{'='*60}")
        print(f"Benchmarking clone: {name} ({url})")
        print(f"{'='*60}")

        for strategy in CLONE_STRATEGIES:
            label = strategy["label"]
            dest = os.path.join(WORK_DIR, f"{name}_{label}")

            if os.path.exists(dest):
                shutil.rmtree(dest)

            elapsed, success = _run_clone(url, dest, strategy["extra_args"])

            size_mb = 0.0
            git_size_mb = 0.0
            if success and os.path.isdir(dest):
                size_mb = _dir_size_mb(dest)
                git_dir = os.path.join(dest, ".git")
                if os.path.isdir(git_dir):
                    git_size_mb = _dir_size_mb(git_dir)

            result = {
                "repo": name,
                "strategy": label,
                "elapsed_s": round(elapsed, 2),
                "success": success,
                "total_size_mb": size_mb,
                "git_dir_size_mb": git_size_mb,
            }
            all_results.append(result)
            print(f"  {label}: {elapsed:.2f}s | total={size_mb} MB | .git={git_size_mb} MB")

            if os.path.exists(dest):
                shutil.rmtree(dest)

    out_path = os.path.join(RESULTS_DIR, "clone_results.json")
    with open(out_path, "w") as f:
        json.dump(all_results, f, indent=2)
    print(f"\nResults written to {out_path}")

    if os.path.exists(WORK_DIR):
        shutil.rmtree(WORK_DIR)

    return all_results


if __name__ == "__main__":
    custom_repos = None
    if len(sys.argv) > 1:
        custom_repos = [
            {"name": url.rstrip("/").split("/")[-1].replace(".git", ""), "url": url}
            for url in sys.argv[1:]
        ]
    bench_clone(custom_repos)
