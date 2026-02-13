#!/bin/bash

# Простой скрипт для миграции Git в Fossil
# Использование: ./migrate-git-to-fossil.sh <URL_репозитория>

GIT_REPO_URL="$1"
REPO_NAME=$(basename "$GIT_REPO_URL" .git)

# Этап 1: Клонирование bare-репозитория
git clone --mirror "$GIT_REPO_URL" "${REPO_NAME}-bare.git"

# Этап 2: Экспорт в fast-export
git --git-dir="${REPO_NAME}-bare.git" fast-export --all --reencode=yes --signed-tags=strip --tag-of-filtered-object=drop > "${REPO_NAME}-export.txt"

# Этап 3: Импорт в Fossil
fossil import --git --force "${REPO_NAME}.fossil" "${REPO_NAME}-export.txt"

# Этап 4: Создание рабочей директории
mkdir "${REPO_NAME}-workspace"
cd "${REPO_NAME}-workspace"
fossil open "../${REPO_NAME}.fossil"
