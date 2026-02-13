#!/bin/bash

# Скрипт для удаления всех PR веток из Fossil репозитория
# Использование: ./remove-pr-branches.sh <путь_к_fossil_workspace>

FOSSIL_WORKSPACE="$1"

if [ -z "$FOSSIL_WORKSPACE" ]; then
    echo "Использование: $0 <путь_к_fossil_workspace>"
    exit 1
fi

cd "$FOSSIL_WORKSPACE" || exit 1

echo "Получение списка PR веток..."
# Удаляем все ветки, содержащие "/", кроме основных веток Git
MAIN_BRANCHES="amlog|jch|maint|master|next|seen|todo"
PR_BRANCHES=$(fossil branch list | grep -E "/" | grep -vE "^[[:space:]]*($MAIN_BRANCHES)$" | sed 's/^[[:space:]]*//')

TOTAL=$(echo "$PR_BRANCHES" | wc -l | tr -d ' ')
echo "Найдено $TOTAL PR веток для удаления"
echo

echo "Скрытие PR веток..."
COUNT=0
for branch in $PR_BRANCHES; do
    fossil branch hide "$branch" > /dev/null 2>&1
    COUNT=$((COUNT + 1))
    if [ $((COUNT % 100)) -eq 0 ]; then
        echo "Скрыто $COUNT из $TOTAL веток..."
    fi
done

echo "Скрытие завершено. Скрыто $COUNT веток"
echo

echo "Проверка оставшихся веток:"
fossil branch list | wc -l | tr -d ' '
echo "веток осталось"
