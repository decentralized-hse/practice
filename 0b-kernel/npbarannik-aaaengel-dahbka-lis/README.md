# git-cofiles

## Авторы:
Баранник Никита, Тимижев Даниил, Энгель Данила

Небольшая CLI утилита на C, которая анализирует историю Git и показывает, какие файлы чаще всего изменялись в тех же коммитах, что и указанный файл.

## Использование
`./git-cofiles <file>` из корня репозитория

## Пример
Для примера взял первый попавшийся репозиторий https://github.com/is-a-good-dev/cli/tree/main

Запускаем git-cofiles из корня репозитория

```
../git-cofiles ./src/functions/account.js
Top 5 files co-changed with "src/functions/account.js"  (1 commit scanned)

  1.  src/util/questions.js                                         1x
  2.  package.json                                                  1x
  3.  src/functions/check.js                                        1x
  4.  src/functions/login.js                                        1x
  5.  src/functions/logout.js                                       1x
```

Тесты лежат в папке tests

Алгоритм:
1. Приводит переданный путь к каноническому repo-relative пути (через `git ls-files`).
2. Собирает список коммитов (без merge), где фигурирует этот файл (`git log --no-merges --follow`).
3. Для каждого такого коммита получает список изменённых файлов (`git diff-tree --name-only`) и считает, какие из них менялись вместе с целевым файлом.
4. Сортирует по убыванию и печатает Top N (по умолчанию 5).

## Требования

- Git (утилита вызывает `git ...` через `popen`)
- Компилятор C (gcc/clang)
- Запуск изнутри git-репозитория
- Файл должен быть отслеживаемым Git

## Сборка

### Linux/macOS

```bash
gcc -O2 -Wall -Wextra -std=c11 -o git-cofiles git-cofiles.c
