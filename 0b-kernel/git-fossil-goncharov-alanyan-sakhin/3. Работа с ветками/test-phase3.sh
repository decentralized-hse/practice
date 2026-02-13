#!/bin/bash

# Скрипт для тестирования операций Фазы 3: сравнение работы с ветками Git и Fossil
# Использование: ./test-phase3.sh <путь_к_bare_git> <путь_к_fossil_workspace>

BARE_GIT_DIR="$1"
FOSSIL_WORKSPACE="$2"

if [ -z "$BARE_GIT_DIR" ] || [ -z "$FOSSIL_WORKSPACE" ]; then
    echo "Использование: $0 <путь_к_bare_git> <путь_к_fossil_workspace>"
    exit 1
fi

echo "=== Этап 3: Сравнение операций работы с ветками ==="
echo

# 3.1. Просмотр веток
echo "3.1. Операция: Просмотр веток"
echo "---"
echo "Git branch -a:"
git --git-dir="$BARE_GIT_DIR" branch -a | head -10
echo

echo "Git branch -v:"
git --git-dir="$BARE_GIT_DIR" branch -v | head -10
echo

echo "Fossil branch list:"
(cd "$FOSSIL_WORKSPACE" && fossil branch list | head -20)
echo

echo "Производительность просмотра веток:"
echo -n "  Git branch -a:   "
time git --git-dir="$BARE_GIT_DIR" branch -a > /dev/null 2>&1
echo -n "  Fossil branch list: "
(cd "$FOSSIL_WORKSPACE" && time fossil branch list > /dev/null 2>&1)
echo

# 3.2. Создание веток (на тестовых репозиториях)
echo "3.2. Операция: Создание веток"
echo "---"
echo "Создание тестовых репозиториев для проверки создания веток..."
TEST_DIR=$(mktemp -d)
cd "$TEST_DIR" || exit 1

# Git тест
mkdir git-test && cd git-test
git init > /dev/null 2>&1
echo "test" > file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1

echo "Git: создание ветки"
echo -n "  git branch new-feature: "
time git branch new-feature > /dev/null 2>&1
git branch | grep new-feature && echo "  [OK] Ветка создана"
echo

cd ..

# Fossil тест
mkdir fossil-test && cd fossil-test
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "test" > file.txt
fossil add file.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1

echo "Fossil: создание ветки"
echo -n "  fossil branch new new-feature trunk: "
time fossil branch new new-feature trunk > /dev/null 2>&1
fossil branch list | grep new-feature && echo "  [OK] Ветка создана"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test 2>/dev/null || true

# 3.3. Удаление/закрытие веток
echo "3.3. Операция: Удаление/закрытие веток"
echo "---"
cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test 2>/dev/null || true

# Git тест
mkdir git-test && cd git-test
git init > /dev/null 2>&1
echo "test" > file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1
git branch test-delete > /dev/null 2>&1

echo "Git: удаление ветки"
echo -n "  git branch -d test-delete: "
time git branch -d test-delete > /dev/null 2>&1
git branch | grep test-delete || echo "  [OK] Ветка удалена"
echo

cd ..

# Fossil тест
mkdir fossil-test && cd fossil-test
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "test" > file.txt
fossil add file.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1
fossil branch new test-close trunk > /dev/null 2>&1

echo "Fossil: закрытие ветки"
echo -n "  fossil branch close test-close: "
time fossil branch close test-close > /dev/null 2>&1
fossil branch list | grep test-close && echo "  [OK] Ветка закрыта (но не удалена)"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test 2>/dev/null || true

# 3.4. Слияние веток
echo "3.4. Операция: Слияние веток"
echo "---"
cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test 2>/dev/null || true

# Git тест
mkdir git-test && cd git-test
git init > /dev/null 2>&1
echo "base" > file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Base" > /dev/null 2>&1
git checkout -b feature > /dev/null 2>&1
echo "feature" >> file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Feature" > /dev/null 2>&1
git checkout main > /dev/null 2>&1

echo "Git: слияние ветки"
echo -n "  git merge feature: "
time git merge feature --no-edit > /dev/null 2>&1
git log --oneline -3 && echo "  [OK] Слияние выполнено"
echo

cd ..

# Fossil тест
mkdir fossil-test && cd fossil-test
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "base" > file.txt
fossil add file.txt > /dev/null 2>&1
fossil commit -m "Base" > /dev/null 2>&1
fossil update trunk > /dev/null 2>&1
echo "feature" >> file.txt
fossil commit --branch feature -m "Feature" > /dev/null 2>&1
fossil update trunk > /dev/null 2>&1

echo "Fossil: слияние ветки"
echo -n "  fossil merge feature: "
time fossil merge feature > /dev/null 2>&1
fossil commit -m "Merge feature" > /dev/null 2>&1
fossil timeline -n 3 && echo "  [OK] Слияние выполнено"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test 2>/dev/null || true

# 3.5. Конфликты слияния
echo "3.5. Операция: Конфликты слияния"
echo "---"
cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test 2>/dev/null || true

# Git тест конфликта
mkdir git-test && cd git-test
git init > /dev/null 2>&1
echo "base" > conflict.txt
git add conflict.txt > /dev/null 2>&1
git commit -m "Base" > /dev/null 2>&1
git checkout -b conflict-branch > /dev/null 2>&1
echo "conflict line" > conflict.txt
git add conflict.txt > /dev/null 2>&1
git commit -m "Conflict commit" > /dev/null 2>&1
git checkout main > /dev/null 2>&1
echo "main line" > conflict.txt
git add conflict.txt > /dev/null 2>&1
git commit -m "Main commit" > /dev/null 2>&1

echo "Git: конфликт слияния"
git merge conflict-branch 2>&1 | grep -i conflict || echo "  (нет конфликта)"
if [ -f conflict.txt ] && grep -q "<<<<<<< HEAD" conflict.txt; then
    echo "  Формат конфликта Git:"
    head -5 conflict.txt
    echo "  [OK] Конфликт обнаружен"
fi
echo

cd ..

# Fossil тест конфликта
mkdir fossil-test && cd fossil-test
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "base" > conflict.txt
fossil add conflict.txt > /dev/null 2>&1
fossil commit -m "Base" > /dev/null 2>&1
fossil update trunk > /dev/null 2>&1
echo "conflict line" > conflict.txt
fossil commit --branch conflict-branch -m "Conflict commit" > /dev/null 2>&1
fossil update trunk > /dev/null 2>&1
echo "main line" > conflict.txt
fossil commit -m "Main commit" > /dev/null 2>&1

echo "Fossil: конфликт слияния"
fossil merge conflict-branch 2>&1 | grep -i conflict || echo "  (нет конфликта)"
if [ -f conflict.txt ] && grep -q "<<<<<<< BEGIN MERGE CONFLICT" conflict.txt; then
    echo "  Формат конфликта Fossil:"
    head -8 conflict.txt
    echo "  [OK] Конфликт обнаружен"
fi
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test 2>/dev/null || true
rmdir "$TEST_DIR" 2>/dev/null || true

echo "=== Тестирование завершено ==="
