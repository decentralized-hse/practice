# git_stats — Git Contributor Statistics

Analyses any Git repository and prints contributor stats: who commits, how often, and what kind of work they do (bugfixes, features, docs, tests, refactors).

## Build

```bash
gcc -O2 -o git_stats header-script.c

---

## Usage
Detailed information
./git_stats --help
---

## Output

============================================================
  Git Contributor Stats  |  linux.git @ v7.0-rc2
============================================================

Total commits               : 10000
  Merge commits             : 1243  (12.4%)
  Code / doc commits        : 8757  (87.6%)
Total unique contributors   : 1891

-- By commit frequency ----------------------------------------
  One-time contributors     :   743  ( 39.3% of contributors)
  Repeat committers         :  1148  ( 60.7% of contributors)
...
The report includes:

- Commit frequency — one-time vs repeat contributors
- Commit type breakdown — bugfix-only, feature-only, mixed, docs, tests, refactor
- Top 30 contributors table sorted by commit count, with per-type columns

---

## Argument reference

| Argument | Required | Description |
|----------|----------|-------------|
| url or path | No | Remote URL or local repo path. Defaults to current directory |
| ref | No | Branch, tag, or commit to analyse. Defaults to HEAD |
| depth | No | Shallow clone depth (number of commits). 0 = full history |

---

## Notes

- Commit classification is based on keywords in the commit subject line and is intentionally broad (e.g. any commit containing "fix" counts as a bugfix). It is a heuristic, not a guarantee.
- Merge commits are counted separately and excluded from type classification.
- Remote clones are placed in /tmp/git_stats_<pid> and deleted automatically after the run.
- The script uses --filter=blob:none when cloning, so only commit metadata is downloaded — not file contents.

```
