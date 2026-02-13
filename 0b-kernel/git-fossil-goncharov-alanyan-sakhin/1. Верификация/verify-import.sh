#!/bin/bash

# Скрипт для верификации корректности импорта Git -> Fossil
# Использование: ./verify-import.sh <путь_к_bare_git> <путь_к_fossil_workspace>

BARE_GIT_DIR="$1"
FOSSIL_WORKSPACE="$2"

echo "=== Этап 1: Верификация корректности импорта ==="
echo

# 1.1. Проверка количества коммитов
echo "1.1. Проверка количества коммитов:"
GIT_COMMITS=$(git --git-dir="$BARE_GIT_DIR" rev-list --all --count)
FOSSIL_COMMITS=$(cd "$FOSSIL_WORKSPACE" && fossil sql "SELECT COUNT(*) FROM event WHERE type='ci';" | tail -1)
echo "  Git:   $GIT_COMMITS коммитов"
echo "  Fossil: $FOSSIL_COMMITS коммитов"
echo

# 1.2. Сравнение количества веток
echo "1.2. Сравнение количества веток:"
GIT_BRANCHES=$(git --git-dir="$BARE_GIT_DIR" for-each-ref --format='%(refname:short)' refs/heads/ refs/remotes/ | wc -l | tr -d ' ')
FOSSIL_BRANCHES=$(cd "$FOSSIL_WORKSPACE" && fossil branch list | wc -l | tr -d ' ')
echo "  Git:   $GIT_BRANCHES веток"
echo "  Fossil: $FOSSIL_BRANCHES веток"
echo

# 1.3. Сравнение количества тегов
echo "1.3. Сравнение количества тегов:"
GIT_TAGS=$(git --git-dir="$BARE_GIT_DIR" tag | wc -l | tr -d ' ')
FOSSIL_TAGS=$(cd "$FOSSIL_WORKSPACE" && fossil tag list | wc -l | tr -d ' ')
echo "  Git:   $GIT_TAGS тегов"
echo "  Fossil: $FOSSIL_TAGS тегов"
echo

# 1.4. Проверка первого коммита
echo "1.4. Проверка первого коммита:"
GIT_FIRST=$(git --git-dir="$BARE_GIT_DIR" log --all --format="%H" | tail -1)
GIT_FIRST_MSG=$(git --git-dir="$BARE_GIT_DIR" log --all --format="%s" | tail -1)
FOSSIL_FIRST_MSG=$(cd "$FOSSIL_WORKSPACE" && fossil sql "SELECT substr(event.comment, 1, 50) FROM blob JOIN event ON blob.rid=event.objid WHERE event.type='ci' ORDER BY event.mtime ASC LIMIT 1;" | tail -1 | tr -d "'")
echo "  Git первый коммит:   $GIT_FIRST"
echo "  Git сообщение:       $GIT_FIRST_MSG"
echo "  Fossil сообщение:    $FOSSIL_FIRST_MSG"
echo

# 1.5. Проверка последнего коммита
echo "1.5. Проверка последнего коммита:"
GIT_LAST=$(git --git-dir="$BARE_GIT_DIR" log --all --format="%H" | head -1)
GIT_LAST_MSG=$(git --git-dir="$BARE_GIT_DIR" log --all --format="%s" | head -1)
FOSSIL_LAST=$(cd "$FOSSIL_WORKSPACE" && fossil info | grep "checkout:" | awk '{print $2}')
echo "  Git последний коммит:   $GIT_LAST"
echo "  Git сообщение:          $GIT_LAST_MSG"
echo "  Fossil последний:       $FOSSIL_LAST"
echo

# 1.6. Валидация содержимого файла (abspath.c на последнем коммите)
echo "1.6. Валидация содержимого файла abspath.c:"
GIT_FILE_HASH=$(git --git-dir="$BARE_GIT_DIR" show "$GIT_LAST":abspath.c 2>/dev/null | md5 | awk '{print $1}')
FOSSIL_FILE_HASH=$(cd "$FOSSIL_WORKSPACE" && cat abspath.c 2>/dev/null | md5 | awk '{print $1}')
if [ -n "$GIT_FILE_HASH" ] && [ -n "$FOSSIL_FILE_HASH" ]; then
    echo "  Git MD5:   $GIT_FILE_HASH"
    echo "  Fossil MD5: $FOSSIL_FILE_HASH"
    if [ "$GIT_FILE_HASH" = "$FOSSIL_FILE_HASH" ]; then
        echo "  [OK] Файлы идентичны"
    else
        echo "  [ERROR] Файлы различаются"
    fi
else
    echo "  [WARNING] Файл не найден в одном из репозиториев"
fi
echo

echo "=== Верификация завершена ==="
