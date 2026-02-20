#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./ort-demo.sh ort
#   ./ort-demo.sh resolve

MODE="${1:-}"
if [[ "$MODE" != "ort" && "$MODE" != "resolve" ]]; then
  echo "Usage: $0 {ort|resolve}"
  exit 1
fi

REPO_DIR="ort-demo-$MODE"

echo "[1/8] Creating demo directory: $REPO_DIR"
rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR"
cd "$REPO_DIR"

echo "[2/8] Initializing git repository"
git init -b main >/dev/null

echo "[3/8] Creating base file"
echo "line 1" > file-a
echo "line 2" >> file-a
git add file-a
git commit -m "Initial file-a" >/dev/null

echo "[4/8] Creating rename-branch (file-a -> file-c)"
git switch -c rename-branch >/dev/null
git mv file-a file-c
git commit -m "Rename file-a to file-c" >/dev/null

echo "[5/8] Creating edit-branch from main (edit file-a)"
git switch main >/dev/null
git switch -c edit-branch >/dev/null
cat > file-a <<'TXT'
line 1 edited on edit-branch
line 2
TXT
git add file-a
git commit -m "Edit file-a on edit-branch" >/dev/null

echo "[6/8] Merging rename-branch into main"
git switch main >/dev/null
git merge --no-ff --no-edit rename-branch >/dev/null

echo "[7/8] Trying merge with -s $MODE"
if [[ "$MODE" == "resolve" ]]; then
  set +e
  git merge --no-ff --no-edit -s resolve edit-branch >/tmp/resolve-merge.log 2>&1
  MERGE_EXIT=$?
  set -e
  if [[ "$MERGE_EXIT" -eq 0 ]]; then
    echo "resolve merge unexpectedly succeeded"
  else
    echo "resolve failed as expected"
    sed -n '1,30p' /tmp/resolve-merge.log
  fi
else
  git merge --no-ff --no-edit -s ort edit-branch >/tmp/ort-merge.log 2>&1
  echo "ort succeeded"
  sed -n '1,20p' /tmp/ort-merge.log
fi

echo "[8/8] Final history and file content"
git --no-pager log --oneline --graph --decorate --all
echo
if [[ -f file-c ]]; then
  echo "file-c content:"
  cat file-c
fi
