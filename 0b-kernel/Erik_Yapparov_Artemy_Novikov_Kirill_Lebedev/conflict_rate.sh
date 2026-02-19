#!/bin/bash
# Usage: ./conflict_rate.sh <repo_path> <date_from> <date_to>
# Example: ./conflict_rate.sh ./linux-kernel 2025-12-01 2025-12-07
set -euo pipefail

REPO="$1"
FROM="$2"
TO="$3"
BEFORE=$(date -d "$TO + 1 day" +%Y-%m-%d)

cd "$REPO"

total=0
conflicts=0

for hash in $(git log --after="$FROM" --before="$BEFORE" --merges --format="%H"); do
    total=$((total + 1))

    p1=$(git log -1 --format="%P" "$hash" | awk '{print $1}')
    p2=$(git log -1 --format="%P" "$hash" | awk '{print $2}')
    [ -z "$p2" ] && continue

    base=$(git merge-base "$p1" "$p2" 2>/dev/null || true)
    [ -z "$base" ] && continue

    if git merge-tree "$base" "$p1" "$p2" 2>/dev/null | grep -q "^+<<<<<<< \.our"; then
        conflicts=$((conflicts + 1))
    fi

    [ $((total % 20)) -eq 0 ] && echo "$total processed..." >&2
done

echo "Merges: $total"
echo "Conflicts: $conflicts"
echo "Rate: $(awk "BEGIN{printf \"%.2f%%\", $conflicts/$total*100}")"
