# kernel-diff-tools

Two utilities for analyzing where changes happen between Linux kernel versions
(or any git repo).

## The problem

- Full kernel clone is ~4 GB
- Diff between even minor versions touches thousands of files
- You just want to know *which subsystems changed and by how much*

## Solution

| Tool | Purpose |
|------|---------|
| `diff-analyzer` | C++ tool — reads `git diff --numstat`, groups by directory, prints stats |
| `kernel-fetch.sh` | Shell script — does a **blobless partial clone** (~500 MB vs 4 GB) of just the two tags you need |

### How the efficient fetch works

```
Full clone:          ~4.0 GB   (all commits, trees, blobs)
Blobless clone:      ~0.5 GB   (commits + trees, blobs fetched on demand)
Treeless clone:      ~0.2 GB   (commits only, trees + blobs on demand)
```

`kernel-fetch.sh` uses `--filter=blob:none` by default. Git lazily fetches only
the file contents (`--numstat` needs) during the diff. Much faster than cloning
everything.

## Quick start

```bash
# Build
make

# Fetch two kernel versions efficiently
./kernel-fetch.sh v6.11 v6.12

# Analyze
cd linux-diff
../diff-analyzer v6.11 v6.12

# Or with options
../diff-analyzer -d2 -n15 v6.11 v6.12    # depth=2 dirs, top 15
```

### Pipe mode (works with any repo)

```bash
cd /path/to/any-git-repo
git diff --numstat main..feature | diff-analyzer -d1 -
```

## diff-analyzer options

```
-d, --depth N     Directory grouping depth (default: 1)
-n, --top N       Show only top N dirs (rest collapsed into "(other)")
-h, --help        Help
```

## Install (optional)

```bash
sudo make install          # installs to /usr/local/bin
sudo make install PREFIX=~/.local  # or local
```

## Example output

```
================================================================================================
  Git Diff Analysis   |   1.8M lines changed across 18421 files   (++1.1M  --742K)
================================================================================================

Directory              Files     Added   Removed     Total     %  Distribution
------------------------------------------------------------------------------------------------
drivers/               6842     420K     310K       730K   39.2% |############              |
arch/                  2103     180K     120K       300K   16.1% |#####                     |
net/                   1504      95K      72K       167K    9.0% |###                       |
fs/                     982      65K      48K       113K    6.1% |##                        |
sound/                  640      52K      30K        82K    4.4% |#                         |
...
```