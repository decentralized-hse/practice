#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./ff-demo.sh fast-forward
#   ./ff-demo.sh ort

MODE="${1:-}"
if [[ "$MODE" != "fast-forward" && "$MODE" != "ort" ]]; then
  echo "Usage: $0 {fast-forward|ort}"
  exit 1
fi

REPO_DIR="ff-demo-$MODE"

echo "[1/7] Creating demo directory: $REPO_DIR"
rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR"
cd "$REPO_DIR"

echo "[2/7] Initializing git repository"
git init -b main >/dev/null

echo "[3/7] Creating initial file"
echo "line 1" > notes.txt
git add notes.txt
git commit -m "Initial commit" >/dev/null

echo "[4/7] Creating and switching to branch: feature"
git switch -c feature >/dev/null

echo "[5/7] Updating file on feature branch"
echo "line 2 from feature" >> notes.txt
git add notes.txt
git commit -m "Feature change" >/dev/null

echo "[6/7] Switching back to main"
git switch main >/dev/null

echo "[7/7] Running merge: $MODE"
if [[ "$MODE" == "fast-forward" ]]; then
  git merge --ff-only feature
else
  git merge --no-ff --no-edit -s ort feature
fi

echo
echo "Merge complete. Final history:"
git --no-pager log --oneline --graph --decorate --all
