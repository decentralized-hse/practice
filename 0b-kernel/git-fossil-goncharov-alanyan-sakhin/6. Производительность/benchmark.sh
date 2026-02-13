#!/bin/bash

# Скрипт для автоматизации бенчмарков производительности Git vs Fossil
# Использование: ./benchmark.sh <путь_к_bare_git> <путь_к_fossil_workspace>

BARE_GIT_DIR="$1"
FOSSIL_WORKSPACE="$2"

if [ -z "$BARE_GIT_DIR" ] || [ -z "$FOSSIL_WORKSPACE" ]; then
    echo "Использование: $0 <путь_к_bare_git> <путь_к_fossil_workspace>"
    exit 1
fi

echo "=== Бенчмарки производительности Git vs Fossil ==="
echo
echo "Тестовый репозиторий: Git (196,325 коммитов)"
echo

# Функция для измерения времени
measure_time() {
    local cmd="$1"
    local desc="$2"
    echo -n "  $desc: "
    time eval "$cmd" > /dev/null 2>&1
}

# 1. Просмотр истории
echo "1. Просмотр истории коммитов"
echo "---"

echo "100 коммитов:"
measure_time "git --git-dir=\"$BARE_GIT_DIR\" log --oneline -100" "Git"
measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil timeline --limit 100)" "Fossil"
echo

echo "1,000 коммитов:"
measure_time "git --git-dir=\"$BARE_GIT_DIR\" log --oneline -1000" "Git"
measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil timeline --limit 1000)" "Fossil"
echo

echo "10,000 коммитов:"
measure_time "git --git-dir=\"$BARE_GIT_DIR\" log --oneline -10000" "Git"
measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil timeline --limit 10000)" "Fossil"
echo

# 2. Просмотр веток
echo "2. Просмотр веток"
echo "---"
measure_time "git --git-dir=\"$BARE_GIT_DIR\" branch -a" "Git branch -a"
measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil branch list)" "Fossil branch list"
echo

# 3. Просмотр тегов
echo "3. Просмотр тегов"
echo "---"
measure_time "git --git-dir=\"$BARE_GIT_DIR\" tag" "Git tag"
measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil tag list)" "Fossil tag list"
echo

# 4. Просмотр коммита
echo "4. Просмотр информации о коммите"
echo "---"
COMMIT=$(git --git-dir="$BARE_GIT_DIR" log --format="%H" -1)
FOSSIL_UUID=$(cd "$FOSSIL_WORKSPACE" && fossil info | grep checkout | awk '{print $2}')
measure_time "git --git-dir=\"$BARE_GIT_DIR\" show --stat $COMMIT" "Git show"
measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil info $FOSSIL_UUID)" "Fossil info"
echo

# 5. Поиск в файле (если файл существует)
echo "5. Поиск в файле"
echo "---"
if [ -f "$FOSSIL_WORKSPACE/abspath.c" ]; then
    measure_time "git --git-dir=\"$BARE_GIT_DIR\" grep 'abspath' -- abspath.c" "Git grep"
    measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil grep 'abspath' abspath.c)" "Fossil grep"
else
    echo "  Файл abspath.c не найден, пропускаем тест"
fi
echo

# 6. Diff между коммитами
echo "6. Просмотр diff между коммитами"
echo "---"
COMMIT=$(git --git-dir="$BARE_GIT_DIR" log --format="%H" -1)
FOSSIL_UUID=$(cd "$FOSSIL_WORKSPACE" && fossil info | grep checkout | awk '{print $2}')
FOSSIL_PARENT=$(cd "$FOSSIL_WORKSPACE" && fossil info $FOSSIL_UUID | grep parent | awk '{print $2}')
if [ -n "$FOSSIL_PARENT" ]; then
    measure_time "git --git-dir=\"$BARE_GIT_DIR\" diff ${COMMIT}~1 $COMMIT" "Git diff"
    measure_time "(cd \"$FOSSIL_WORKSPACE\" && fossil diff $FOSSIL_PARENT $FOSSIL_UUID)" "Fossil diff"
else
    echo "  Не удалось определить родительский коммит Fossil, пропускаем тест"
fi
echo

# 7. Статистика репозиториев
echo "7. Статистика репозиториев"
echo "---"
echo "Git:"
git --git-dir="$BARE_GIT_DIR" count-objects -vH | grep -E "(count|size-pack)"
echo "Количество коммитов: $(git --git-dir=\"$BARE_GIT_DIR\" rev-list --all --count)"
echo
echo "Fossil:"
echo "Размер репозитория: $(du -sh \"$FOSSIL_WORKSPACE/../git.fossil\" 2>/dev/null | awk '{print $1}')"
echo "Количество коммитов: $(cd \"$FOSSIL_WORKSPACE\" && fossil sql \"SELECT COUNT(*) FROM event WHERE type='ci';\" | tail -1)"
echo

echo "=== Бенчмарки завершены ==="
