# Write-Ahead Log

## Авторы: 
Окунев Данила, Лавицкая Александра, Яппаров Эрик

В проекте представлена полная реализация WAL: файлы сегментов с двадцатибайтовыми заголовками, буферизация записей с добавлением в конец (append-only), кумулятивные хэш-контрольные точки на основе алгоритма BLAKE3, механизмы восстановления и усечения старых сегментов, ротация сегментов при достижении заданного размера, а также итератор для воспроизведения записей.

### BASON и Assignment 1

По заданию в сегмент пишется **одна закодированная BASON-запись** на логическую запись WAL (плюс выравнивание до 8 байт). Реализация кодека из Assignment 1 в этой ветке **не входит** (её делают другие участники).

Вместо этого в `src/bason_record.cpp` стоит **явная заглушка** `bason_encode` / `bason_try_decode` / `bason_skip_record`. Публичная **модель данных**: `BasonType`, поля `key` / `value` / `children` у `BasonRecord`.

**Как подключить настоящий кодек:** заменить тела функций `bason_encode`, `bason_encode_into`, `bason_try_decode`, `bason_skip_record`, `bason_decode` на вызовы полной реализации Assignment 1 (TLV, short/long, ключи и контейнеры по RFC). WAL менять не нужно — он уже вызывает только `bason_encode` и `bason_try_decode` / `bason_skip_record`.

Заглушка сейчас кодирует только **лист `String` с пустым `key`** (см. `BasonRecord::leaf_string` в тестах). Остальные типы и пути — после интеграции кодека.

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Запуск тестов

```bash
./build/test_basic
./build/test_corruption
./build/test_crash
./build/test_perf
./build/test_rotate
```