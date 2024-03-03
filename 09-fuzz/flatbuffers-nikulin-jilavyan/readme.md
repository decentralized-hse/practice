# 09-fuzz: flatbuffers-nikulin-jilavyan

## Пофикшенные баги
- Отсутствовала валидация данных, добавил ее, позаимствовав часть кода из `bin-gritzko-check.c`.

- В `flatbuffers-nikulin.cpp:110` создавался `unique_ptr<char>`, а на вход подавалась память, выделенная
с помощью `new char[]`. Исправил на `unique_ptr<char[]>`.

- (Бонус) [Пофиксил](https://github.com/decentralized-hse/practice/pull/158) баг в `bin-gritzko-check.c`.

## Зависимости
FlatBuffers последней версии на 2.03.24, libFuzzer.

## Сборка и запуск
Распилил изначальный код на `flatbuffers-nikulin.cpp` и `flatbuffers-nikulin-main.cpp`, чтобы можно
было отдельно собирать таргет для фаззинга.

Сборка таргета для фаззинга:
```
clang++ -g -fsanitize=address,fuzzer fuzz.cpp flatbuffers-nikulin.cpp bin-gritzko-check.cpp
```
Сборка решения для `04-formats`:
```
clang++ flatbuffers-nikulin-main.cpp flatbuffers-nikulin.cpp bin-gritzko-check.cpp
```

В обоих таргетах в зависимости от компилятора может понадобиться добавить флаг `-std=с++17`.
