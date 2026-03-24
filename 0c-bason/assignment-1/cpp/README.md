# BASON Codec Implementation

## Обзор

Реализация кодека BASON (Binary Adaptation of Structured Object Notation) - бинарного формата для JSON-совместимых данных с использованием TLV (Tag-Length-Value) структуры.

Это Assignment 1 из задания "Building Databases with BASON".

## Структура проекта

```
hw3/
├── src/                        # Исходный код библиотеки
│   ├── ron64.{h,cpp}           # RON64 кодирование для индексов
│   ├── path_utils.{h,cpp}      # Утилиты для работы с путями
│   ├── bason_codec.{h,cpp}     # Основной кодек (encode/decode)
│   ├── strictness.{h,cpp}      # Валидатор строгости
│   ├── flatten.{h,cpp}         # Flat/Nested преобразования
│   └── json_converter.{h,cpp}  # JSON <-> BASON конвертер
├── tools/
│   └── basondump.cpp           # CLI утилита для отладки
├── test/                       # Unit-тесты
│   ├── test_ron64.cpp
│   ├── test_path.cpp
│   ├── test_codec.cpp
│   ├── test_json.cpp
│   └── test_flatten.cpp
├── CMakeLists.txt
└── REPORT.md
```

## Сборка

### Требования

- C++17 или новее
- CMake 3.14+
- Компилятор: GCC, Clang или MSVC

### Команды сборки

```bash
mkdir build && cd build
cmake ..
make
```

Это создаст:
- Библиотеку `libbason.a`
- Утилиту `basondump`
- Тесты `test_bason`

### Запуск тестов

```bash
cd build
./test_bason
```

Или через CTest:
```bash
ctest --output-on-failure
```

### CLI утилита basondump

```bash
# Показать hex дамп с аннотациями
./basondump --hex file.bason

# Конвертировать в JSON
./basondump --json file.bason

# Валидация по маске строгости (Standard = 0x1FF)
./basondump --validate=1FF file.bason

# Преобразование nested → flat
./basondump --flatten input.bason > output.bason

# Преобразование flat → nested
./basondump --unflatten input.bason > output.bason
```
