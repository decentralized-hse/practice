#!/usr/bin/env bash
#
# kernel-fetch.sh — Efficiently fetch two Linux kernel tags for diffing.
#
# Strategy: blobless partial clone.  Downloads commits + trees but NOT file
# contents (~500 MB vs ~4 GB for full clone).  When diff-analyzer runs
# `git diff --numstat`, git lazily fetches only the blobs it actually needs.
#
# For really constrained bandwidth there's a --treeless mode that skips trees
# too (~200 MB initial), but the diff itself will be slower.
#
set -euo pipefail

REPO_URL="https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"
WORKDIR="./linux-diff"
FILTER="blob:none"          # default: blobless
TAGS=()

usage() {
    cat <<EOF
Usage: $(basename "$0") [options] <tag1> <tag2>

Efficiently download just enough of the Linux kernel repo to diff two tags.

Options:
  -o, --output DIR     Working directory   (default: ./linux-diff)
  -u, --url URL        Git remote URL      (default: kernel.org)
  -t, --treeless       Use tree:0 filter   (smaller download, slower diff)
  -f, --full           No filter           (full clone, biggest download)
  -h, --help           This message

Examples:
  $(basename "$0") v6.6 v6.7
  $(basename "$0") -o /tmp/kdiff v6.11 v6.12
  $(basename "$0") --treeless v6.0 v6.12

After fetching, run the analyzer:
  cd $WORKDIR && diff-analyzer <tag1> <tag2>
  # or:
  cd $WORKDIR && git diff --numstat <tag1>..<tag2> | diff-analyzer -
EOF
    exit "${1:-0}"
}

log()  { echo "==> $*"; }
warn() { echo "WARNING: $*" >&2; }
die()  { echo "ERROR: $*" >&2; exit 1; }

# ── Parse args ───────────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        -o|--output)   WORKDIR="$2"; shift 2 ;;
        -u|--url)      REPO_URL="$2"; shift 2 ;;
        -t|--treeless) FILTER="tree:0"; shift ;;
        -f|--full)     FILTER=""; shift ;;
        -h|--help)     usage 0 ;;
        -*)            die "Unknown option: $1" ;;
        *)             TAGS+=("$1"); shift ;;
    esac
done

[[ ${#TAGS[@]} -eq 2 ]] || { echo "Error: exactly 2 tags required"; usage 1; }

TAG1="${TAGS[0]}"
TAG2="${TAGS[1]}"

# ── Validate tag format (basic safety) ───────────────────────────────────────
validate_tag() {
    local tag="$1"
    if [[ ! "$tag" =~ ^v?[0-9]+\.[0-9]+([.-].*)?$ ]]; then
        warn "Tag '$tag' doesn't look like a kernel version — proceeding anyway"
    fi
}
validate_tag "$TAG1"
validate_tag "$TAG2"

# ── Check for existing repo ──────────────────────────────────────────────────
if [[ -d "$WORKDIR/.git" ]] || [[ -f "$WORKDIR/HEAD" ]]; then
    log "Repo exists at $WORKDIR, fetching tags..."
    cd "$WORKDIR"

    # Make sure remote URL matches
    CURRENT_URL=$(git config --get remote.origin.url 2>/dev/null || true)
    if [[ -n "$CURRENT_URL" && "$CURRENT_URL" != "$REPO_URL" ]]; then
        warn "Existing remote ($CURRENT_URL) differs from requested ($REPO_URL)"
        warn "Updating remote origin URL"
        git remote set-url origin "$REPO_URL"
    fi

    # Fetch just the two tags we need
    log "Fetching $TAG1 and $TAG2..."
    git fetch --depth=1 origin "refs/tags/$TAG1:refs/tags/$TAG1" \
                                "refs/tags/$TAG2:refs/tags/$TAG2" 2>&1 || {
        # depth fetch for tags can fail if they're annotated; try without depth
        warn "Shallow tag fetch failed, trying full tag fetch..."
        git fetch origin "refs/tags/$TAG1:refs/tags/$TAG1" \
                         "refs/tags/$TAG2:refs/tags/$TAG2" 2>&1
    }
else
    # ── Fresh clone ──────────────────────────────────────────────────────────
    log "Creating minimal clone in $WORKDIR"
    log "Remote: $REPO_URL"
    log "Filter: ${FILTER:-none (full clone)}"
    log "Tags:   $TAG1  $TAG2"
    echo ""

    mkdir -p "$WORKDIR"

    CLONE_ARGS=(
        clone
        --bare               # no working tree needed
        --no-checkout
        --single-branch      # don't fetch all branches
        --no-tags            # we'll fetch specific tags below
    )

    if [[ -n "$FILTER" ]]; then
        CLONE_ARGS+=(--filter="$FILTER")
    fi

    # Clone with minimal data — use one of the tags as the branch hint
    # (we just need a valid ref to satisfy --single-branch)
    log "Step 1/2: Cloning repo structure..."
    git "${CLONE_ARGS[@]}" --branch "$TAG1" "$REPO_URL" "$WORKDIR" 2>&1

    cd "$WORKDIR"

    # Now fetch the second tag
    log "Step 2/2: Fetching second tag ($TAG2)..."
    git fetch origin "refs/tags/$TAG2:refs/tags/$TAG2" 2>&1
fi

# ── Verify tags exist ────────────────────────────────────────────────────────
for tag in "$TAG1" "$TAG2"; do
    if ! git rev-parse --verify "$tag" &>/dev/null; then
        die "Tag '$tag' not found in repo. Check spelling."
    fi
done

# ── Print summary ────────────────────────────────────────────────────────────
REPO_SIZE=$(du -sh . 2>/dev/null | cut -f1)

echo ""
log "Ready!  Repo size on disk: $REPO_SIZE"
log ""
log "Run the analyzer:"
log "  diff-analyzer -d1 $TAG1 $TAG2              # from inside $WORKDIR"
log "  # or pipe mode:"
log "  git diff --numstat $TAG1..$TAG2 | diff-analyzer -d2 -n20 -"
echo ""