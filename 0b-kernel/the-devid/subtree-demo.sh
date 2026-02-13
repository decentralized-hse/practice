#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./subtree-demo.sh subtree
#   ./subtree-demo.sh ort

MODE="${1:-}"
if [[ "$MODE" != "subtree" && "$MODE" != "ort" ]]; then
  echo "Usage: $0 {subtree|ort}"
  exit 1
fi

CORE_REPO="subtree-demo-core-$MODE"
DEMO_REPO="subtree-demo-app-$MODE"

echo "[1/7] Creating core repository: $CORE_REPO"
rm -rf "$CORE_REPO" "$DEMO_REPO"
mkdir -p "$CORE_REPO"
cd "$CORE_REPO"
git init -b main >/dev/null

echo "really important text" > core.txt
git add core.txt
git commit -m "Initial core content" >/dev/null

cd ..

echo "[2/7] Creating second repository: $DEMO_REPO"
mkdir -p "$DEMO_REPO"
cd "$DEMO_REPO"
git init -b main >/dev/null
echo "Demo app" > README.md
git add README.md
git commit -m "Initial app commit" >/dev/null

if [[ "$MODE" == "subtree" ]]; then
  echo "[3/7] Importing core with subtree workflow"
  git remote add core "../$CORE_REPO"
  git fetch core >/dev/null
  git merge -s ours --no-commit --allow-unrelated-histories core/main >/dev/null
  git read-tree --prefix=vendor/core -u core/main
  git commit -m "Initial subtree import of core" >/dev/null
else
  echo "[3/7] Importing core with copy-paste workflow"
  mkdir -p vendor/core
  cp "../$CORE_REPO/core.txt" vendor/core/core.txt
  git add vendor/core/core.txt
  git commit -m "Initial copy-paste snapshot" >/dev/null
fi

cd ..

echo "[4/7] Adding a few commits to $CORE_REPO"
cd "$CORE_REPO"
echo "really important text v2" > core.txt
git add core.txt
git commit -m "Core update: v2 text" >/dev/null

echo "integration contract: stable" > contract.txt
git add contract.txt
git commit -m "Core update: add contract" >/dev/null

echo "really important text v3" > core.txt
git add core.txt
git commit -m "Core update: v3 text" >/dev/null

cd ..
cd "$DEMO_REPO"

if [[ "$MODE" == "subtree" ]]; then
  echo "[5/7] Updating with -s subtree"
  git fetch core >/dev/null
  git merge --no-ff --no-edit --allow-unrelated-histories -s subtree -Xsubtree=vendor/core core/main >/dev/null
else
  echo "[5/7] Updating with copy-paste + commit"
  cp "../$CORE_REPO/core.txt" vendor/core/core.txt
  cp "../$CORE_REPO/contract.txt" vendor/core/contract.txt
  git add vendor/core
  git commit -m "Update by copy-paste" >/dev/null
fi

echo "[6/7] Showing final files"
find vendor -maxdepth 3 -type f | sort

echo "[7/7] Showing history"
git --no-pager log --oneline --graph --decorate
