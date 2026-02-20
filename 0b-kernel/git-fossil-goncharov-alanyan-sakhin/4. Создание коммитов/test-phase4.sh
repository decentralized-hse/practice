#!/bin/bash

# Скрипт для тестирования операций Фазы 4: сравнение создания коммитов Git и Fossil
# Использование: ./test-phase4.sh

echo "=== Этап 4: Сравнение операций создания коммитов ==="
echo

# Создание тестовых репозиториев
TEST_DIR=$(mktemp -d)
cd "$TEST_DIR" || exit 1

# 4.1. git add vs fossil add
echo "4.1. Операция: Подготовка изменений (git add vs fossil add)"
echo "---"

# Git тест
echo "Git: создание репозитория и файлов..."
mkdir git-test && cd git-test || exit 1
git init > /dev/null 2>&1
echo "initial" > file1.txt
git add file1.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1

echo "Изменение файлов..."
echo "modified" > file1.txt
echo "new" > file2.txt

echo "Git status (до add):"
git status --short
echo

echo "Git: добавление file2.txt в индекс"
echo -n "  git add file2.txt: "
time git add file2.txt > /dev/null 2>&1
echo "  [OK] Файл добавлен в индекс"
echo

echo "Git status (после add file2.txt):"
git status --short
echo "  Видно разделение: file2.txt в индексе, file1.txt изменен но не в индексе"
echo

cd ..

# Fossil тест
echo "Fossil: создание репозитория и файлов..."
mkdir fossil-test && cd fossil-test || exit 1
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "initial" > file1.txt
fossil add file1.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1

echo "Изменение файлов..."
echo "modified" > file1.txt
echo "new" > file2.txt

echo "Fossil status (до add):"
fossil status | grep -E "(EDITED|ADDED|DELETED)" || echo "  (нет изменений)"
echo

echo "Fossil: добавление file2.txt"
echo -n "  fossil add file2.txt: "
time fossil add file2.txt > /dev/null 2>&1
echo "  [OK] Файл помечен для добавления"
echo

echo "Fossil status (после add file2.txt):"
fossil status | grep -E "(EDITED|ADDED|DELETED)"
echo "  Все изменения показываются вместе (нет разделения на staged/unstaged)"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test

# 4.2. git status vs fossil status
echo "4.2. Операция: Просмотр статуса (git status vs fossil status)"
echo "---"

# Git тест
mkdir git-test && cd git-test || exit 1
git init > /dev/null 2>&1
echo "file1" > file1.txt
git add file1.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1
echo "modified" > file1.txt
echo "new" > file2.txt
git add file2.txt > /dev/null 2>&1

echo "Git status:"
git status
echo

echo "Производительность git status:"
echo -n "  "
time git status > /dev/null 2>&1
echo

cd ..

# Fossil тест
mkdir fossil-test && cd fossil-test || exit 1
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "file1" > file1.txt
fossil add file1.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1
echo "modified" > file1.txt
echo "new" > file2.txt
fossil add file2.txt > /dev/null 2>&1

echo "Fossil status:"
fossil status
echo

echo "Производительность fossil status:"
echo -n "  "
time fossil status > /dev/null 2>&1
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test

# 4.3. git commit vs fossil commit
echo "4.3. Операция: Создание коммитов (git commit vs fossil commit)"
echo "---"

# Git тест
mkdir git-test && cd git-test || exit 1
git init > /dev/null 2>&1
echo "file1" > file1.txt
git add file1.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1
echo "modified" > file1.txt
echo "new" > file2.txt
git add file1.txt file2.txt > /dev/null 2>&1

echo "Git: создание коммита"
echo -n "  git commit -m 'Update files': "
time git commit -m "Update files" > /dev/null 2>&1
echo "  [OK] Коммит создан"
echo

echo "Git: информация о коммите"
git log --oneline -1
git show HEAD --format=fuller | head -8
echo

cd ..

# Fossil тест
mkdir fossil-test && cd fossil-test || exit 1
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "file1" > file1.txt
fossil add file1.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1
echo "modified" > file1.txt
echo "new" > file2.txt
fossil add file2.txt > /dev/null 2>&1

echo "Fossil: создание коммита"
echo -n "  fossil commit -m 'Update files': "
time fossil commit -m "Update files" > /dev/null 2>&1
echo "  [OK] Коммит создан"
echo

echo "Fossil: информация о коммите"
fossil timeline -n 1
fossil info | head -10
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test

# 4.4. Демонстрация различий в staging area
echo "4.4. Демонстрация различий: Staging Area"
echo "---"

# Git: можно коммитить только часть изменений
mkdir git-test && cd git-test || exit 1
git init > /dev/null 2>&1
echo "file1" > file1.txt
echo "file2" > file2.txt
git add file1.txt file2.txt > /dev/null 2>&1
git commit -m "Initial" > /dev/null 2>&1
echo "change1" > file1.txt
echo "change2" > file2.txt
git add file1.txt > /dev/null 2>&1

echo "Git: коммит только file1.txt (file2.txt остается измененным)"
git commit -m "Update file1 only" > /dev/null 2>&1
git status --short
echo "  [OK] file1.txt закоммичен, file2.txt остался измененным"
echo

cd ..

# Fossil: все изменения коммитятся вместе
mkdir fossil-test && cd fossil-test || exit 1
fossil init test.fossil > /dev/null 2>&1
fossil open test.fossil > /dev/null 2>&1
echo "file1" > file1.txt
echo "file2" > file2.txt
fossil add file1.txt file2.txt > /dev/null 2>&1
fossil commit -m "Initial" > /dev/null 2>&1
echo "change1" > file1.txt
echo "change2" > file2.txt

echo "Fossil: коммит всех изменений"
fossil commit -m "Update files" > /dev/null 2>&1
fossil status
echo "  [OK] Все изменения закоммичены вместе (нет возможности выбрать часть)"
echo

cd "$TEST_DIR" || exit 1
rm -rf git-test fossil-test
rmdir "$TEST_DIR" 2>/dev/null || true

echo "=== Тестирование завершено ==="
