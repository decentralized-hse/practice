#!/usr/bin/env python3
import argparse
import logging
import os
import subprocess
import sys
from typing import Dict, Optional, Tuple


OFFICIAL_REPO_URL = "https://github.com/torvalds/linux.git"
LOGGER = logging.getLogger(__name__)


def run(cmd, cwd=None) -> str:
    LOGGER.debug("Running command: %s", " ".join(cmd))
    result = subprocess.run(
        cmd,
        cwd=cwd,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return result.stdout


def git_cmd(args, git_dir: Optional[str]) -> list:
    if git_dir:
        return ["git", "--git-dir", git_dir, *args]
    return ["git", *args]


def ensure_repo(repo_path: str, refresh: bool) -> None:
    if os.path.isdir(os.path.join(repo_path, ".git")):
        if refresh:
            LOGGER.info("Fetching updates in %s", repo_path)
            run(git_cmd(["fetch", "--all", "--tags", "--prune"], None), cwd=repo_path)
        return

    os.makedirs(os.path.dirname(repo_path), exist_ok=True)
    LOGGER.info("Cloning official Linux repo to %s", repo_path)
    run(
        git_cmd(
            ["clone", "--no-tags", "--filter=blob:none", OFFICIAL_REPO_URL, repo_path],
            None,
        )
    )


def resolve_git_dir(git_file: str) -> str:
    if os.path.isdir(git_file):
        return git_file
    if os.path.isfile(git_file):
        with open(git_file, "r", encoding="utf-8") as handle:
            data = handle.read().strip()
        if data.startswith("gitdir:"):
            raw_path = data.split(":", 1)[1].strip()
            if not os.path.isabs(raw_path):
                raw_path = os.path.join(os.path.dirname(git_file), raw_path)
            if os.path.isdir(raw_path):
                return raw_path
    raise FileNotFoundError(f"Invalid git file or directory: {git_file}")


def email_to_org(email: str) -> str:
    email = email.strip().lower()
    if "@" not in email:
        return "unknown"
    return email.split("@", 1)[1]


def get_commit_count(repo_path: Optional[str], git_dir: Optional[str]) -> int:
    output = run(git_cmd(["rev-list", "--count", "HEAD"], git_dir), cwd=repo_path).strip()
    return int(output) if output else 0


def collect_stats(
    repo_path: Optional[str],
    git_dir: Optional[str],
    progress_every: int,
) -> Dict[str, Dict[str, int]]:
    stats: Dict[str, Dict[str, int]] = {}
    current_org = None

    location = git_dir if git_dir else repo_path
    LOGGER.info("Collecting commit stats from %s", location)
    total_commits = None
    try:
        total_commits = get_commit_count(repo_path, git_dir)
    except (ValueError, subprocess.CalledProcessError):
        LOGGER.debug("Could not determine total commit count.")

    cmd = git_cmd(
        [
            "log",
            "--numstat",
            "--format=@@@%ae",
        ],
        git_dir,
    )
    cmd.extend(["--no-renames", "--no-ext-diff"])
    process = subprocess.Popen(
        cmd,
        cwd=repo_path,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

    assert process.stdout is not None
    commit_index = 0
    for line in process.stdout:
        line = line.rstrip("\n")
        if line.startswith("@@@"):
            email = line[3:]
            current_org = email_to_org(email)
            org_stats = stats.setdefault(
                current_org,
                {"commits": 0, "added": 0, "deleted": 0, "total": 0},
            )
            org_stats["commits"] += 1
            commit_index += 1
            if progress_every > 0 and commit_index % progress_every == 0:
                if total_commits:
                    percent = (commit_index / total_commits) * 100
                    LOGGER.info(
                        "Progress: %d/%d commits (%.1f%%)",
                        commit_index,
                        total_commits,
                        percent,
                    )
                else:
                    LOGGER.info("Progress: %d commits", commit_index)
            continue

        if not line or current_org is None:
            continue

        parts = line.split("\t")
        if len(parts) < 3:
            continue
        added_s, deleted_s = parts[0], parts[1]
        if added_s == "-" or deleted_s == "-":
            continue
        try:
            added = int(added_s)
            deleted = int(deleted_s)
        except ValueError:
            continue

        org_stats = stats[current_org]
        org_stats["added"] += added
        org_stats["deleted"] += deleted
        org_stats["total"] += added + deleted

    process.wait()
    if process.returncode != 0:
        stderr = process.stderr.read() if process.stderr else ""
        raise RuntimeError(f"git log failed: {stderr.strip()}")

    if total_commits and commit_index and commit_index != total_commits:
        LOGGER.info("Progress: %d/%d commits (100%%)", commit_index, total_commits)
    LOGGER.info("Aggregated stats for %d orgs", len(stats))
    return stats


def format_table(rows, headers):
    widths = [len(h) for h in headers]
    for row in rows:
        for i, col in enumerate(row):
            widths[i] = max(widths[i], len(str(col)))

    def fmt_row(row):
        return "  ".join(str(col).ljust(widths[i]) for i, col in enumerate(row))

    lines = [fmt_row(headers), fmt_row(["-" * w for w in widths])]
    lines.extend(fmt_row(row) for row in rows)
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Top N kernel contributors aggregated by org (email domain)."
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable verbose logging.",
    )
    parser.add_argument(
        "--repo-path",
        default=os.path.join(os.getcwd(), "linux-github"),
        help="Path to linux repo clone (default: ./linux-github).",
    )
    parser.add_argument(
        "--git-file",
        help="Path to .git directory or gitfile (overrides --repo-path).",
    )
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="Fetch updates if the repo already exists.",
    )
    parser.add_argument(
        "--top",
        type=int,
        default=100,
        help="Number of orgs to show (default: 100).",
    )
    parser.add_argument(
        "--loc-metric",
        choices=["added", "deleted", "total"],
        default="total",
        help="LoC metric for secondary sorting (default: total).",
    )
    parser.add_argument(
        "--progress-every",
        type=int,
        default=5000,
        help="Log progress every N commits (default: 5000).",
    )

    args = parser.parse_args()

    logging.basicConfig(
        level=logging.INFO if args.verbose else logging.WARNING,
        format="%(levelname)s: %(message)s",
    )

    git_dir = None
    repo_path = args.repo_path
    if args.git_file:
        git_dir = resolve_git_dir(args.git_file)
        repo_path = None
    else:
        ensure_repo(repo_path, args.refresh)

    stats = collect_stats(repo_path, git_dir, args.progress_every)

    def sort_key(item: Tuple[str, Dict[str, int]]):
        org, s = item
        return (-s["commits"], -s[args.loc_metric], org)

    sorted_items = sorted(stats.items(), key=sort_key)[: args.top]

    rows = []
    for idx, (org, s) in enumerate(sorted_items, start=1):
        rows.append(
            [
                idx,
                org,
                s["commits"],
                s["added"],
                s["deleted"],
                s["total"],
            ]
        )

    headers = ["#", "org", "commits", "added", "deleted", "total"]
    print(format_table(rows, headers))
    return 0


if __name__ == "__main__":
    sys.exit(main())
