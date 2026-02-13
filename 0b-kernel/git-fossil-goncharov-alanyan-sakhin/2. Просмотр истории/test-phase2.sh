#!/bin/bash

# Скрипт для тестирования операций Фазы 2: сравнение Git и Fossil
# Использование: ./test-phase2.sh <путь_к_bare_git> <путь_к_fossil_workspace>

BARE_GIT_DIR="$1"
FOSSIL_WORKSPACE="$2"
ORIGINAL_DIR=$(pwd)

echo "=== Этап 2: Сравнение операций просмотра и навигации ==="
echo

# 2.1. git log vs fossil timeline
echo "2.1. Операция: git log vs fossil timeline"
echo "----------------------------------------"

echo "Git log --oneline -10:"
git --git-dir="$BARE_GIT_DIR" log --oneline -10
echo

echo "Fossil timeline --limit 10:"
(cd "$FOSSIL_WORKSPACE" && fossil timeline --limit 10)
echo

echo "Производительность (100 коммитов):"
echo -n "  Git:   "
time git --git-dir="$BARE_GIT_DIR" log --oneline -100 > /dev/null 2>&1
echo -n "  Fossil: "
(cd "$FOSSIL_WORKSPACE" && time fossil timeline --limit 100 > /dev/null 2>&1)
echo

echo "Производительность (1000 коммитов):"
echo -n "  Git:   "
time git --git-dir="$BARE_GIT_DIR" log --oneline -1000 > /dev/null 2>&1
echo -n "  Fossil: "
(cd "$FOSSIL_WORKSPACE" && time fossil timeline --limit 1000 > /dev/null 2>&1)
echo

echo "Производительность (10000 коммитов):"
echo -n "  Git:   "
time git --git-dir="$BARE_GIT_DIR" log --oneline -10000 > /dev/null 2>&1
echo -n "  Fossil: "
(cd "$FOSSIL_WORKSPACE" && time fossil timeline --limit 10000 > /dev/null 2>&1)
echo

echo "Производительность (все коммиты):"
echo -n "  Git:   "
time git --git-dir="$BARE_GIT_DIR" log --oneline --all > /dev/null 2>&1
echo -n "  Fossil: "
(cd "$FOSSIL_WORKSPACE" && time fossil timeline --limit 200000 > /dev/null 2>&1)
echo

echo "Git log --format:"
git --git-dir="$BARE_GIT_DIR" log --format="%H|%an|%ae|%ad|%s" -5
echo

echo "Git log --author:"
git --git-dir="$BARE_GIT_DIR" log --author="Junio C Hamano" --oneline -5
echo

echo "Fossil timeline --user:"
(cd "$FOSSIL_WORKSPACE" && fossil timeline --user "gitster@pobox.com" --limit 5)
echo

# 2.2. git show vs fossil diff/cat/info
echo
echo "2.2. Операция: git show vs fossil diff/cat/info"
echo "----------------------------------------------"

LAST_GIT=$(git --git-dir="$BARE_GIT_DIR" log --format="%H" -1 HEAD)
echo "Git show --stat (последний коммит):"
git --git-dir="$BARE_GIT_DIR" show "$LAST_GIT" --stat | head -15
echo

LAST_FOSSIL=$(cd "$FOSSIL_WORKSPACE" && fossil info | grep "checkout:" | awk '{print $2}')
echo "Fossil info (последний коммит):"
(cd "$FOSSIL_WORKSPACE" && fossil info "$LAST_FOSSIL" | head -15)
echo

echo "Производительность git show:"
time git --git-dir="$BARE_GIT_DIR" show "$LAST_GIT" > /dev/null
echo

echo "Git show <commit>:<file>:"
git --git-dir="$BARE_GIT_DIR" show "$LAST_GIT":abspath.c | head -10
echo

echo "Fossil cat <file> -r <commit>:"
(cd "$FOSSIL_WORKSPACE" && fossil cat abspath.c -r "$LAST_FOSSIL" | head -10)
echo

# 2.3. git blame vs fossil annotate
echo
echo "2.3. Операция: git blame vs fossil annotate"
echo "-------------------------------------------"

echo "Git blame abspath.c (первые 20 строк):"
git --git-dir="$BARE_GIT_DIR" blame abspath.c | head -20
echo

echo "Fossil annotate abspath.c (первые 20 строк):"
(cd "$FOSSIL_WORKSPACE" && fossil annotate abspath.c | head -20)
echo

echo "Производительность:"
echo -n "  Git blame:   "
time git --git-dir="$BARE_GIT_DIR" blame abspath.c > /dev/null
echo -n "  Fossil annotate: "
(cd "$FOSSIL_WORKSPACE" && time fossil annotate abspath.c > /dev/null)
echo

echo "Git blame --show-email:"
git --git-dir="$BARE_GIT_DIR" blame --show-email abspath.c | head -10
echo

echo "Git blame -L 10,20:"
git --git-dir="$BARE_GIT_DIR" blame -L 10,20 abspath.c
echo

# 2.4. git diff vs fossil diff
echo
echo "2.4. Операция: git diff vs fossil diff"
echo "--------------------------------------"

COMMIT1=$(git --git-dir="$BARE_GIT_DIR" log --format="%H" -3 HEAD | head -1)
COMMIT2=$(git --git-dir="$BARE_GIT_DIR" log --format="%H" -3 HEAD | tail -1)

echo "Git diff между двумя коммитами:"
git --git-dir="$BARE_GIT_DIR" diff "$COMMIT2" "$COMMIT1" | head -30
echo

echo "Git diff --stat:"
git --git-dir="$BARE_GIT_DIR" diff --stat "$COMMIT2" "$COMMIT1"
echo

echo "Git diff --name-status:"
git --git-dir="$BARE_GIT_DIR" diff --name-status "$COMMIT2" "$COMMIT1"
echo

echo "Git diff --numstat:"
git --git-dir="$BARE_GIT_DIR" diff --numstat "$COMMIT2" "$COMMIT1"
echo

echo "Производительность git diff:"
time git --git-dir="$BARE_GIT_DIR" diff "$COMMIT2" "$COMMIT1" > /dev/null
echo

# Для Fossil нужно найти соответствующие коммиты
echo "Fossil diff (используя коммиты из timeline):"
(cd "$FOSSIL_WORKSPACE" && FOSSIL_C1=$(fossil timeline --limit 3 | grep -oE '\[[a-f0-9]{10,}\]' | head -1 | tr -d '[]') && FOSSIL_C2=$(fossil timeline --limit 3 | grep -oE '\[[a-f0-9]{10,}\]' | head -2 | tail -1 | tr -d '[]') && echo "Попытка сравнить: $FOSSIL_C2 -> $FOSSIL_C1" && fossil diff "$FOSSIL_C2" "$FOSSIL_C1" 2>&1 | head -30 || echo "Примечание: Fossil требует полные UUID, не сокращенные хеши")
echo

echo "=== Тестирование завершено ==="
