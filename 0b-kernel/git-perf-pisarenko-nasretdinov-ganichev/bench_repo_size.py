"""
bench_repo_size.py - Analyze repository size characteristics.

Measures:
  - Total .git directory size
  - Packfile sizes
  - Loose object count
  - git count-objects -vH
  - Total commit count
  - Total file count in working tree

Requires existing local clones. Set REPO_PATHS or pass paths as CLI args.

Writes results to results/repo_size_results.json
"""

import json
import os
import subprocess
import sys

RESULTS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results")

DEFAULT_REPO_PATHS = [
    {"name": "linux", "path": "/home/grigoriypisar/HW/repos/linux"},
    {"name": "homebrew-core", "path": "/home/grigoriypisar/HW/repos/homebrew-core"},
    {"name": "crates.io-index", "path": "/home/grigoriypisar/HW/repos/crates.io-index"},
]


def _dir_size_bytes(path):
    """Recursively compute directory size in bytes."""
    total = 0
    for dirpath, _dirnames, filenames in os.walk(path):
        for f in filenames:
            fp = os.path.join(dirpath, f)
            if os.path.isfile(fp):
                total += os.path.getsize(fp)
    return total


def _run_git(args, cwd):
    """Run a git command and return stdout or None on failure."""
    try:
        result = subprocess.run(
            ["git"] + args, cwd=cwd, capture_output=True, text=False, check=True
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def _parse_count_objects(output):
    """Parse output of git count-objects -v into a dict."""
    data = {}
    if not output:
        return data
    for line in output.splitlines():
        if b":" in line:
            key, val = line.split(b":", 1)
            key = key.strip().replace(b" ", b"_").replace(b"-", b"_")
            val = val.strip()
            try:
                data[key.decode("utf-8")] = int(val)
            except ValueError:
                data[key.decode("utf-8")] = val
    return data


def _count_packfiles(git_dir):
    """Count and measure packfiles in .git/objects/pack/."""
    pack_dir = os.path.join(git_dir, "objects", "pack")
    packs = []
    if os.path.isdir(pack_dir):
        for f in os.listdir(pack_dir):
            if f.endswith(".pack"):
                fp = os.path.join(pack_dir, f)
                packs.append({
                    "name": f,
                    "size_mb": round(os.path.getsize(fp) / (1024 * 1024), 2),
                })
    return packs


def bench_repo_size(repo_paths=None):
    repo_paths = repo_paths or DEFAULT_REPO_PATHS
    if not repo_paths:
        print("No repository paths configured.")
        print("Usage: python bench_repo_size.py /path/to/repo1 [/path/to/repo2 ...]")
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
        print(f"Analyzing repo size: {name} ({path})")
        print(f"{'='*60}")

        git_dir = os.path.join(path, ".git")
        if not os.path.isdir(git_dir):
            print(f"  No .git directory found in {path}")
            continue

        git_size_bytes = _dir_size_bytes(git_dir)
        git_size_mb = round(git_size_bytes / (1024 * 1024), 2)
        print(f"  .git size: {git_size_mb} MB")

        total_size_bytes = _dir_size_bytes(path)
        total_size_mb = round(total_size_bytes / (1024 * 1024), 2)
        print(f"  Total repo size: {total_size_mb} MB")

        wt_size_mb = round((total_size_bytes - git_size_bytes) / (1024 * 1024), 2)
        print(f"  Working tree size: {wt_size_mb} MB")

        co_output = _run_git(["count-objects", "-v"], cwd=path)
        co_data = _parse_count_objects(co_output)
        print(f"  count-objects: {co_data}")

        packs = _count_packfiles(git_dir)
        total_pack_mb = sum(p["size_mb"] for p in packs)
        print(f"  Packfiles: {len(packs)} (total {total_pack_mb} MB)")

        commit_count_str = _run_git(["rev-list", "--count", "HEAD"], cwd=path)
        commit_count = int(commit_count_str) if commit_count_str else 0
        print(f"  Commits: {commit_count}")

        ls_output = _run_git(["ls-files"], cwd=path)
        file_count = len(ls_output.splitlines()) if ls_output else 0
        print(f"  Tracked files: {file_count}")

        branch_output = _run_git(["branch", "-a"], cwd=path)
        branch_count = len(branch_output.splitlines()) if branch_output else 0

        tag_output = _run_git(["tag", "-l"], cwd=path)
        tag_count = len(tag_output.splitlines()) if tag_output else 0
        print(f"  Branches: {branch_count} | Tags: {tag_count}")

        result = {
            "repo": name,
            "git_dir_size_mb": git_size_mb,
            "total_size_mb": total_size_mb,
            "working_tree_size_mb": wt_size_mb,
            "commit_count": commit_count,
            "tracked_file_count": file_count,
            "branch_count": branch_count,
            "tag_count": tag_count,
            "packfile_count": len(packs),
            "packfile_total_mb": total_pack_mb,
            "packfiles": packs,
            "count_objects": co_data,
        }
        all_results.append(result)

    out_path = os.path.join(RESULTS_DIR, "repo_size_results.json")
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
    bench_repo_size(custom)
