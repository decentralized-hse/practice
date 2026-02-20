"""
plot_results.py - Generate PNG charts from benchmark JSON results.

Reads all *_results.json files from the results/ directory and produces
corresponding PNG graphs in results/graphs/.

Graphs produced:
  1. Clone time comparison (grouped bar chart by repo & strategy)
  2. Clone .git size comparison
  3. Log operation times (grouped bar chart)
  4. Blame time vs file size (scatter plot)
  5. Ref enumeration times (bar chart)
  6. Repo size overview (stacked bar chart)
"""

import json
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

RESULTS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results")
GRAPHS_DIR = os.path.join(RESULTS_DIR, "graphs")


def _load_json(filename):
    path = os.path.join(RESULTS_DIR, filename)
    if not os.path.isfile(path):
        return None
    with open(path, "r") as f:
        return json.load(f)


def plot_clone():
    """Plot clone benchmark results."""
    data = _load_json("clone_results.json")
    if not data:
        print("No clone_results.json found, skipping clone plots.")
        return

    repos = []
    seen = set()
    for r in data:
        if r["repo"] not in seen:
            repos.append(r["repo"])
            seen.add(r["repo"])

    strategies = ["full", "shallow", "partial"]
    x = np.arange(len(repos))
    width = 0.25

    fig, ax = plt.subplots(figsize=(10, 6))
    for i, strat in enumerate(strategies):
        times = []
        for repo in repos:
            entry = next((r for r in data if r["repo"] == repo and r["strategy"] == strat), None)
            times.append(entry["elapsed_s"] if entry and entry["success"] else 0)
        ax.bar(x + i * width, times, width, label=strat)

    ax.set_xlabel("Repository")
    ax.set_ylabel("Time (seconds)")
    ax.set_title("Git Clone Time by Strategy")
    ax.set_xticks(x + width)
    ax.set_xticklabels(repos, rotation=15, ha="right")
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "clone_time.png"), dpi=150)
    plt.close(fig)
    print("  Saved clone_time.png")

    fig, ax = plt.subplots(figsize=(10, 6))
    for i, strat in enumerate(strategies):
        sizes = []
        for repo in repos:
            entry = next((r for r in data if r["repo"] == repo and r["strategy"] == strat), None)
            sizes.append(entry["git_dir_size_mb"] if entry and entry["success"] else 0)
        ax.bar(x + i * width, sizes, width, label=strat)

    ax.set_xlabel("Repository")
    ax.set_ylabel(".git Size (MB)")
    ax.set_title("Git Clone - .git Directory Size by Strategy")
    ax.set_xticks(x + width)
    ax.set_xticklabels(repos, rotation=15, ha="right")
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "clone_size.png"), dpi=150)
    plt.close(fig)
    print("  Saved clone_size.png")


def plot_log():
    """Plot log benchmark results."""
    data = _load_json("log_results.json")
    if not data:
        print("No log_results.json found, skipping log plots.")
        return

    repos = []
    seen = set()
    for r in data:
        if r["repo"] not in seen:
            repos.append(r["repo"])
            seen.add(r["repo"])

    common_ops = [
        "rev-list --count HEAD",
        "log --oneline (full)",
        "log -n 100 --oneline",
        "log -n 1000 --oneline",
        "log -n 10000 --oneline",
        "shortlog -sn --no-merges",
    ]

    fig, ax = plt.subplots(figsize=(12, 6))
    x = np.arange(len(common_ops))
    width = 0.8 / max(len(repos), 1)

    for i, repo in enumerate(repos):
        times = []
        for op in common_ops:
            entry = next((r for r in data if r["repo"] == repo and r["operation"] == op), None)
            times.append(entry["elapsed_s"] if entry and entry["success"] else 0)
        ax.bar(x + i * width, times, width, label=repo)

    ax.set_xlabel("Operation")
    ax.set_ylabel("Time (seconds)")
    ax.set_title("Git Log Operations - Time by Repository")
    ax.set_xticks(x + width * len(repos) / 2)
    ax.set_xticklabels(common_ops, rotation=25, ha="right", fontsize=8)
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "log_operations.png"), dpi=150)
    plt.close(fig)
    print("  Saved log_operations.png")

    path_entries = [r for r in data if r["operation"].startswith("log --oneline -- ")]
    if path_entries:
        fig, ax = plt.subplots(figsize=(12, 6))
        labels = [f"{r['repo']}: {r['operation'].replace('log --oneline -- ', '')}" for r in path_entries]
        times = [r["elapsed_s"] for r in path_entries]
        line_counts = [r["output_lines"] for r in path_entries]

        bars = ax.barh(range(len(labels)), times, color="steelblue")
        ax.set_yticks(range(len(labels)))
        ax.set_yticklabels(labels, fontsize=8)
        ax.set_xlabel("Time (seconds)")
        ax.set_title("Path-Filtered Git Log - Time per File")
        ax.grid(axis="x", alpha=0.3)

        for idx, (bar, lc) in enumerate(zip(bars, line_counts)):
            ax.text(bar.get_width() + 0.1, bar.get_y() + bar.get_height() / 2,
                    f"{lc} commits", va="center", fontsize=7, color="gray")

        fig.tight_layout()
        fig.savefig(os.path.join(GRAPHS_DIR, "log_path_filtered.png"), dpi=150)
        plt.close(fig)
        print("  Saved log_path_filtered.png")


def plot_blame():
    """Plot blame benchmark results."""
    data = _load_json("blame_results.json")
    if not data:
        print("No blame_results.json found, skipping blame plots.")
        return

    successful = [r for r in data if r["success"]]
    if not successful:
        print("  No successful blame results to plot.")
        return

    fig, ax = plt.subplots(figsize=(10, 6))

    repos = list(set(r["repo"] for r in successful))
    colors = plt.cm.tab10(np.linspace(0, 1, max(len(repos), 1)))

    for idx, repo in enumerate(repos):
        entries = [r for r in successful if r["repo"] == repo]
        x = [r["file_lines"] for r in entries]
        y = [r["elapsed_s"] for r in entries]
        labels = [r["file"].split("/")[-1] for r in entries]

        ax.scatter(x, y, color=colors[idx], label=repo, s=60, alpha=0.8)
        for xi, yi, lbl in zip(x, y, labels):
            ax.annotate(lbl, (xi, yi), fontsize=6, alpha=0.7,
                        xytext=(5, 5), textcoords="offset points")

    ax.set_xlabel("File Size (lines)")
    ax.set_ylabel("Blame Time (seconds)")
    ax.set_title("Git Blame - Time vs File Size")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "blame_scatter.png"), dpi=150)
    plt.close(fig)
    print("  Saved blame_scatter.png")

    fig, ax = plt.subplots(figsize=(12, 6))
    labels = [f"{r['repo']}: {r['file']}" for r in successful]
    times = [r["elapsed_s"] for r in successful]
    file_sizes = [r["file_lines"] for r in successful]

    bars = ax.barh(range(len(labels)), times, color="coral")
    ax.set_yticks(range(len(labels)))
    ax.set_yticklabels(labels, fontsize=7)
    ax.set_xlabel("Time (seconds)")
    ax.set_title("Git Blame - Time per File")
    ax.grid(axis="x", alpha=0.3)

    for idx, (bar, fs) in enumerate(zip(bars, file_sizes)):
        ax.text(bar.get_width() + 0.1, bar.get_y() + bar.get_height() / 2,
                f"{fs} lines", va="center", fontsize=7, color="gray")

    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "blame_bars.png"), dpi=150)
    plt.close(fig)
    print("  Saved blame_bars.png")


def plot_refs():
    """Plot ref enumeration results."""
    data = _load_json("refs_results.json")
    if not data:
        print("No refs_results.json found, skipping refs plots.")
        return

    local_ops = [r for r in data if not r["operation"].startswith("ls-remote")]
    if not local_ops:
        return

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    labels = [f"{r['repo']}: {r['operation']}" for r in local_ops]
    times = [r["elapsed_s"] for r in local_ops]
    ref_counts = [r["ref_count"] for r in local_ops]

    ax1.barh(range(len(labels)), times, color="mediumpurple")
    ax1.set_yticks(range(len(labels)))
    ax1.set_yticklabels(labels, fontsize=7)
    ax1.set_xlabel("Time (seconds)")
    ax1.set_title("Ref Operations - Time")
    ax1.grid(axis="x", alpha=0.3)

    ax2.barh(range(len(labels)), ref_counts, color="mediumseagreen")
    ax2.set_yticks(range(len(labels)))
    ax2.set_yticklabels(labels, fontsize=7)
    ax2.set_xlabel("Reference Count")
    ax2.set_title("Ref Operations - Count")
    ax2.grid(axis="x", alpha=0.3)

    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "refs_operations.png"), dpi=150)
    plt.close(fig)
    print("  Saved refs_operations.png")

    if len(local_ops) > 2:
        fig, ax = plt.subplots(figsize=(8, 6))
        ax.scatter(ref_counts, times, s=60, color="darkorange", alpha=0.8)
        for lbl, rc, t in zip(labels, ref_counts, times):
            ax.annotate(lbl.split(": ")[1], (rc, t), fontsize=7,
                        xytext=(5, 5), textcoords="offset points")
        ax.set_xlabel("Reference Count")
        ax.set_ylabel("Time (seconds)")
        ax.set_title("Ref Enumeration - Time vs Count")
        ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(os.path.join(GRAPHS_DIR, "refs_scatter.png"), dpi=150)
        plt.close(fig)
        print("  Saved refs_scatter.png")


def plot_repo_size():
    """Plot repository size analysis."""
    data = _load_json("repo_size_results.json")
    if not data:
        print("No repo_size_results.json found, skipping size plots.")
        return

    repos = [r["repo"] for r in data]
    x = np.arange(len(repos))

    fig, ax = plt.subplots(figsize=(10, 6))
    wt_sizes = [r["working_tree_size_mb"] for r in data]
    git_sizes = [r["git_dir_size_mb"] for r in data]

    ax.bar(x, wt_sizes, label="Working Tree", color="skyblue")
    ax.bar(x, git_sizes, bottom=wt_sizes, label=".git Directory", color="salmon")

    ax.set_xlabel("Repository")
    ax.set_ylabel("Size (MB)")
    ax.set_title("Repository Size Breakdown")
    ax.set_xticks(x)
    ax.set_xticklabels(repos, rotation=15, ha="right")
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "repo_size_breakdown.png"), dpi=150)
    plt.close(fig)
    print("  Saved repo_size_breakdown.png")

    if len(data) > 1:
        fig, ax = plt.subplots(figsize=(8, 6))
        commits = [r["commit_count"] for r in data]
        sizes = [r["git_dir_size_mb"] for r in data]
        ax.scatter(commits, sizes, s=100, color="teal", alpha=0.8)
        for repo, cc, sz in zip(repos, commits, sizes):
            ax.annotate(repo, (cc, sz), fontsize=9,
                        xytext=(8, 8), textcoords="offset points")
        ax.set_xlabel("Commit Count")
        ax.set_ylabel(".git Size (MB)")
        ax.set_title("Commit Count vs .git Directory Size")
        ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(os.path.join(GRAPHS_DIR, "size_vs_commits.png"), dpi=150)
        plt.close(fig)
        print("  Saved size_vs_commits.png")

    fig, ax = plt.subplots(figsize=(12, 2 + 0.5 * len(data)))
    ax.axis("off")
    headers = ["Repo", "Commits", "Files", "Tags", "Branches",
               ".git (MB)", "Pack (MB)", "Total (MB)"]
    rows = []
    for r in data:
        rows.append([
            r["repo"],
            f"{r['commit_count']:,}",
            f"{r['tracked_file_count']:,}",
            f"{r['tag_count']:,}",
            f"{r['branch_count']:,}",
            f"{r['git_dir_size_mb']:,.1f}",
            f"{r['packfile_total_mb']:,.1f}",
            f"{r['total_size_mb']:,.1f}",
        ])

    table = ax.table(cellText=rows, colLabels=headers, loc="center",
                     cellLoc="center", colWidths=[0.15] + [0.1] * 7)
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1.2, 1.5)
    ax.set_title("Repository Size Overview", fontsize=12, pad=20)
    fig.tight_layout()
    fig.savefig(os.path.join(GRAPHS_DIR, "repo_size_table.png"), dpi=150)
    plt.close(fig)
    print("  Saved repo_size_table.png")


def main():
    os.makedirs(GRAPHS_DIR, exist_ok=True)
    print(f"Generating graphs in {GRAPHS_DIR}\n")

    plot_clone()
    plot_log()
    plot_blame()
    plot_refs()
    plot_repo_size()

    print(f"\nAll graphs saved to {GRAPHS_DIR}")


if __name__ == "__main__":
    main()
