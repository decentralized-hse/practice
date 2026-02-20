#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./ours-demo.sh resolve
#   ./ours-demo.sh ours

MODE="${1:-}"
if [[ "$MODE" != "resolve" && "$MODE" != "ours" ]]; then
  echo "Usage: $0 {resolve|ours}"
  exit 1
fi

REPO_DIR="ours-demo-$MODE"

echo "[1/8] Creating demo directory: $REPO_DIR"
rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR"
cd "$REPO_DIR"

echo "[2/8] Initializing git repository"
git init -b main >/dev/null

echo "[3/8] Creating base python division script"
cat > division.py <<'PY'
a = float(input())
b = float(input())
print(a / b)
PY
git add division.py
git commit -m "Initial division script" >/dev/null

echo "[4/8] Creating fix-branch"
git switch -c fix-branch >/dev/null
cat > division.py <<'PY'
a = float(input())
b = float(input())
if b == 0:
    print("Error: division by zero")
else:
    print(a / b)
PY
git add division.py
git commit -m "Handle division by zero with error message" >/dev/null

echo "[5/8] Creating enrichment branch from main"
git switch main >/dev/null
git switch -c enrichment >/dev/null
cat > division.py <<'PY'
a = float(input())
b = float(input())
if b == 0:
    if a == 0:
        print("idk")
    else:
        print("\u221E")
else:
    print(a / b)
PY
git add division.py
git commit -m "Handle division by zero with infinity/idk" >/dev/null

echo "[6/8] Merging fix-branch into main"
git switch main >/dev/null
git merge --no-ff --no-edit fix-branch >/dev/null

if [[ "$MODE" == "ours" ]]; then
  echo "[7/8] Merging main into enrichment using -s ours"
  git switch enrichment >/dev/null
  git merge --no-ff --no-edit -s ours main >/dev/null

  echo "[8/8] Merging enrichment back into main"
  git switch main >/dev/null
  git merge enrichment >/dev/null
else
  echo "[7/8] Merging enrichment into main with -s $MODE (expect conflict)"
  set +e
  git merge -s "$MODE" enrichment >/dev/null 2>&1
  MERGE_EXIT=$?
  set -e

  if [[ "$MERGE_EXIT" -eq 0 ]]; then
    echo "Expected a merge conflict, but merge succeeded"
    exit 1
  fi

  echo "[8/8] Conflict shown in division.py"
  cat division.py
fi

echo
echo "Final history:"
git --no-pager log --oneline --graph --decorate --all
