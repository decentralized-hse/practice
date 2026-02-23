## Домашнее задание 1

Take a Linux kernel repo. For any two versions, produce stats (df -kh like): where the changes happened (which dirs, how much).

## Работу выполнили

- Горбацевич Анна (AnnaGorbatsevich)
- Горбатюк Олег (o1eg0)
- Сюй София (sonyasyuy)

## Примеры использования

### 1) Сборка анализатора

```bash
make
```

Соберёт бинарник `diff-analyzer`

### 2) Подготовка репозитория Ядра Linux

```bash
./kernel-fetch.sh v6.6 v6.7
```

По умолчанию создаст bare-репозиторий в `./linux-diff` и скачает только нужные данные

### 3) Анализ изменений между тегами

```bash
cd linux-diff
../diff-analyzer v6.6 v6.7
```

Покажет распределение изменений по директориям (кол-во файлов, добавленные/удалённые строки, процент)

### 4) Группировка глубже и ограничение top-N

```bash
cd linux-diff
../diff-analyzer -d 2 -n 10 v6.6 v6.7
```

- `-d 2`: группировка по директориям на глубине 2
- `-n 10`: показать только 10 самых изменённых директорий (остальные попадут в `(other)`)

### 5) Полный сценарий одной командой

```bash
make
./kernel-fetch.sh v6.11 v6.12
cd linux-diff && ../diff-analyzer -d 1 v6.11 v6.12
```
