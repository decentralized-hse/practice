#!/usr/bin/env bash

# Tests for git-cofiles

set -euo pipefail

BINARY="$(cd "$(dirname "$0")/.." && pwd)/git-cofiles"
TMPDIR_BASE="$(mktemp -d)"
PASS=0
FAIL=0

cleanup() { rm -rf "$TMPDIR_BASE"; }
trap cleanup EXIT

assert_contains() {
    local label="$1" expected="$2" actual="$3"
    if echo "$actual" | grep -qF "$expected"; then
        echo "  PASS: $label"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $label"
        echo "        expected to find : $expected"
        echo "        in output        :"
        echo "$actual" | sed 's/^/          /'
        FAIL=$((FAIL+1))
    fi
}

assert_not_contains() {
    local label="$1" unexpected="$2" actual="$3"
    if ! echo "$actual" | grep -qF "$unexpected"; then
        echo "  PASS: $label"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $label"
        echo "        did not expect   : $unexpected"
        echo "        in output        :"
        echo "$actual" | sed 's/^/          /'
        FAIL=$((FAIL+1))
    fi
}

assert_exit() {
    local label="$1" expected_code="$2" actual_code="$3"
    if [[ "$actual_code" -eq "$expected_code" ]]; then
        echo "  PASS: $label"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $label (exit $actual_code, want $expected_code)"
        FAIL=$((FAIL+1))
    fi
}

echo "=== Building ==="
make -C "$(dirname "$BINARY")" git-cofiles 2>&1
echo ""

#  Commit history for the test repo (newest last):
#
#  c1: alpha.txt beta.txt gamma.txt <- 3 files
#  c2: alpha.txt beta.txt delta.txt <- 3 files
#  c3: alpha.txt beta.txt epsilon.txt zeta.txt
#  c4: alpha.txt gamma.txt <- 2 files
#  c5: beta.txt delta.txt <- does not touch alpha
#
#  Expected co-change counts for alpha.txt:
#  - beta.txt -> 3 (c1, c2, c3)
#  - gamma.txt -> 2 (c1, c4)
#  - delta.txt -> 1 (c2)
#  - epsilon.txt -> 1 (c3)
#  - zeta.txt -> 1 (c3)

REPO="$TMPDIR_BASE/repo"
mkdir -p "$REPO"
git -C "$REPO" init -q
git -C "$REPO" config user.email "test@test.com"
git -C "$REPO" config user.name  "Test"
git -C "$REPO" commit -q --allow-empty -m "init"

commit() {
    for f in "$@"; do
        echo "$f v$RANDOM" > "$REPO/$f"
    done
    git -C "$REPO" add -- "$@"
    git -C "$REPO" commit -q -m "change: $*"
}

commit alpha.txt beta.txt gamma.txt
commit alpha.txt beta.txt delta.txt
commit alpha.txt beta.txt epsilon.txt zeta.txt
commit alpha.txt gamma.txt
commit beta.txt delta.txt

echo ""

# Test 1: basic ranking
echo "=== Test 1: co-change ranking for alpha.txt ==="
out=$(cd "$REPO" && "$BINARY" alpha.txt 2>&1)

assert_contains "beta.txt appears in output"    "beta.txt"    "$out"
assert_contains "gamma.txt appears in output"   "gamma.txt"   "$out"
assert_contains "delta.txt appears in output"   "delta.txt"   "$out"
assert_contains "epsilon.txt appears in output" "epsilon.txt" "$out"
assert_contains "zeta.txt appears in output"    "zeta.txt"    "$out"

assert_contains "beta.txt count is 3"   "3x" "$out"
assert_contains "gamma.txt count is 2"  "2x" "$out"

beta_line=$(echo  "$out" | grep -n "beta.txt"  | cut -d: -f1)
gamma_line=$(echo "$out" | grep -n "gamma.txt" | cut -d: -f1)
if [[ "$beta_line" -lt "$gamma_line" ]]; then
    echo "  PASS: beta.txt ranked above gamma.txt"
    PASS=$((PASS+1))
else
    echo "  FAIL: expected beta.txt before gamma.txt"
    FAIL=$((FAIL+1))
fi

echo ""

# Test 2: file not tracked
echo "=== Test 2: non-tracked file produces error ==="
err=$(cd "$REPO" && "$BINARY" nonexistent.txt 2>&1) || code=$?
assert_contains "error message for untracked file" "not tracked" "$err"
echo ""

# Test 3: no arguments
echo "=== Test 3: wrong argument count ==="
err=$("$BINARY" 2>&1) || code=$?; code=${code:-0}
set +e; "$BINARY" >/dev/null 2>&1; code=$?; set -e
assert_exit  "exit 1 with no args"    1 "$code"
usage_out=$("$BINARY" 2>&1 || true)
assert_contains "usage hint printed" "usage" "$usage_out"
echo ""

# Test 4: file with no co-changes
echo "=== Test 4: file never co-changed with another file ==="
echo "solo" > "$REPO/solo.txt"
git -C "$REPO" add solo.txt
git -C "$REPO" commit -q -m "solo commit"

out=$(cd "$REPO" && "$BINARY" solo.txt 2>&1)
assert_contains "message about no co-changes" "never committed together" "$out"
echo ""

# Test 5: top-5 cap
echo "=== Test 5: output is capped at 5 entries ==="
commit_extra() {
    echo "x" > "$REPO/extra_$1.txt"
    git -C "$REPO" add "extra_$1.txt"
    echo "alpha extra" > "$REPO/alpha.txt"
    git -C "$REPO" add alpha.txt
    git -C "$REPO" commit -q -m "extra $1"
}

for n in a b c d e f; do commit_extra "$n"; done

out=$(cd "$REPO" && "$BINARY" alpha.txt 2>&1)
count=$(echo "$out" | grep -cE '^ +[0-9]+\.' || true)
if [[ "$count" -le 5 ]]; then
    echo "  PASS: at most 5 results shown ($count)"
    PASS=$((PASS+1))
else
    echo "  FAIL: expected ≤5 results, got $count"
    FAIL=$((FAIL+1))
fi
echo ""

# Summary
echo "=== Results: $PASS passed, $FAIL failed ==="
[[ "$FAIL" -eq 0 ]]
