#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./octopus-demo.sh octopus
#   ./octopus-demo.sh multiple-ort

MODE="${1:-}"
if [[ "$MODE" != "octopus" && "$MODE" != "multiple-ort" ]]; then
  echo "Usage: $0 {octopus|multiple-ort}"
  exit 1
fi

REPO_DIR="octopus-demo-$MODE"

echo "[1/8] Creating demo directory: $REPO_DIR"
rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR"
cd "$REPO_DIR"

echo "[2/8] Initializing git repository"
git init -b main >/dev/null

echo "[3/8] Creating initial file"
echo "# Product backlog" > README.md
git add README.md
git commit -m "Initial commit" >/dev/null

echo "[4/8] Creating branch feature-auth"
git switch -c feature-auth >/dev/null
echo "Authentication requirements and login flow" > auth-spec.md
git add auth-spec.md
git commit -m "Add authentication spec" >/dev/null
git switch main >/dev/null

echo "[5/8] Creating branch feature-billing"
git switch -c feature-billing >/dev/null
echo "Billing events and invoicing rules" > billing-spec.md
git add billing-spec.md
git commit -m "Add billing spec" >/dev/null
git switch main >/dev/null

echo "[6/8] Creating branch feature-notifications"
git switch -c feature-notifications >/dev/null
echo "Notification channels and delivery strategy" > notifications-spec.md
git add notifications-spec.md
git commit -m "Add notifications spec" >/dev/null
git switch main >/dev/null

echo "[7/8] Running merge mode: $MODE"
if [[ "$MODE" == "octopus" ]]; then
  git merge --no-edit feature-auth feature-billing feature-notifications
else
  git merge --no-ff --no-edit -s ort feature-auth
  git merge --no-ff --no-edit -s ort feature-billing
  git merge --no-ff --no-edit -s ort feature-notifications
fi

echo "[8/8] Merge complete"
echo
echo "Final history:"
git --no-pager log --oneline --graph --decorate --all
