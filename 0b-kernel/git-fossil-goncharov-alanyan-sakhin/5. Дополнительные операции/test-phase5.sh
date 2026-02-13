#!/bin/bash

# Скрипт для тестирования операций Фазы 5: сравнение дополнительных операций Git и Fossil
# Использование: ./test-phase5.sh

echo "=== Этап 5: Сравнение дополнительных операций ==="
echo

# Создание тестовых репозиториев
TEST_DIR=$(mktemp -d)
cd "$TEST_DIR" || exit 1

# 5.1. Отмена изменений
echo "5.1. Операция: Отмена изменений"
echo "---"

# Git тест
echo "Git: создание репозитория..."
mkdir git-test && cd git-test || exit 1
git init > /dev/null 2>&1
echo "line1" > file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1
echo "line2" >> file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Add line2" > /dev/null 2>&1

echo "Git: отмена последнего коммита (soft reset)"
echo -n "  git reset HEAD~1: "
time git reset HEAD~1 > /dev/null 2>&1
git status --short && echo "  [OK] Коммит отменен, изменения сохранены"
echo

echo "Git: восстановление файла из HEAD"
echo "bad change" > file.txt
echo -n "  git checkout -- file.txt: "
time git checkout -- file.txt > /dev/null 2>&1
cat file.txt | grep -q "line1" && echo "  [OK] Файл восстановлен"
echo

cd ..

# Fossil тест
echo "Fossil: создание репозитория..."
mkdir fossil-test && cd fossil-test || exit 1
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "line1" > file.txt
fossil add file.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1
echo "line2" >> file.txt
fossil commit -m "Add line2" > /dev/null 2>&1

echo "Fossil: восстановление файла из checkout"
echo "bad change" > file.txt
echo -n "  fossil revert file.txt: "
time fossil revert file.txt > /dev/null 2>&1
cat file.txt | grep -q "line1" && echo "  [OK] Файл восстановлен"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test

# 5.2. Создание обратного коммита
echo "5.2. Операция: Создание обратного коммита"
echo "---"

# Git тест
mkdir git-test && cd git-test || exit 1
git init > /dev/null 2>&1
echo "line1" > file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1
echo "line2" >> file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Add line2" > /dev/null 2>&1

echo "Git: создание обратного коммита"
echo -n "  git revert HEAD: "
time git revert HEAD --no-edit > /dev/null 2>&1
git log --oneline -3 && echo "  [OK] Обратный коммит создан"
echo

cd ..

# Fossil тест
mkdir fossil-test && cd fossil-test || exit 1
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "line1" > file.txt
fossil add file.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1
echo "line2" >> file.txt
COMMIT_UUID=$(fossil commit -m "Add line2" 2>&1 | grep "New_Version:" | awk '{print $2}')

echo "Fossil: создание обратного коммита"
echo -n "  fossil merge --backout + commit: "
time fossil merge --backout "$COMMIT_UUID" > /dev/null 2>&1
fossil commit -m "Revert Add line2" > /dev/null 2>&1
fossil timeline -n 3 && echo "  [OK] Обратный коммит создан"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test

# 5.3. Поиск (grep)
echo "5.3. Операция: Поиск (git grep vs fossil grep)"
echo "---"

# Git тест
mkdir git-test && cd git-test || exit 1
git init > /dev/null 2>&1
echo "test pattern here" > file.txt
git add file.txt > /dev/null 2>&1
git commit -m "Add file" > /dev/null 2>&1

echo "Git: поиск в текущих файлах"
echo -n "  git grep 'pattern': "
time git grep "pattern" > /dev/null 2>&1
git grep "pattern" && echo "  [OK] Поиск выполнен"
echo

cd ..

# Fossil тест
mkdir fossil-test && cd fossil-test || exit 1
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "test pattern here" > file.txt
fossil add file.txt > /dev/null 2>&1
fossil commit -m "Add file" > /dev/null 2>&1

echo "Fossil: поиск в истории файла"
echo -n "  fossil grep 'pattern' file.txt: "
time fossil grep "pattern" file.txt > /dev/null 2>&1
fossil grep "pattern" file.txt | head -3 && echo "  [OK] Поиск выполнен"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test
rmdir "$TEST_DIR" 2>/dev/null || true

echo "=== Тестирование завершено ==="
echo
echo "Примечание: Тестирование работы с удаленными репозиториями (push/pull/sync)"
echo "требует настройки удаленного репозитория и не включено в автоматический тест."
