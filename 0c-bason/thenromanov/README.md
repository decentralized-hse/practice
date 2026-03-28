# Assignment 6: BASONLevel

## Описание

Реализация задания 6 из курса: LSM-дерево с leveled compaction поверх бинарного формата BASON.

## Архитектура

```
Запись:  WAL.append → memtable.put → (freeze → flush → L0 SST)
Чтение:  memtable → frozen → L0 (новые первыми) → L1+ (binary search)
Удаление: tombstone (Boolean с пустым value)
```

**Compaction**: Leveled compaction запускается фоновым потоком; L0→L1 при превышении порога файлов; L(N)→L(N+1) при превышении размера уровня.

**Snapshot**: фиксирует WAL offset; чтения через snapshot видят данные на момент создания. Без snapshot `max_offset = std::numeric_limits<uint64_t>::max()` (видны все записи). С snapshot `max_offset = snapshot->offset`. Snapshot'ы хранятся в двусвязном списке `SnapshotList`.

**MANIFEST**: это WAL, хранящий историю изменений набора SST файлов. Каждая запись - `VersionEdit` в формате BASON Object:
```
{
  "add": [{level, file, file_size, first_key, last_key, min_offset, max_offset}, ...],
  "remove": [{level, file}, ...],
  "flush_offset": <WAL offset>
}
```

При открытии базы MANIFEST воспроизводится для восстановления `Version` (текущий набор SST файлов по уровням).

**SST**: SST файлы хранят internal keys вместо user keys:

```
internal_key = user_key + '\0' + 8-byte big-endian (UINT64_MAX - offset)
```

Инвертирование offset обеспечивает порядок (key ASC, offset DESC) при лексикографической сортировке. Это значит, что самая новая версия ключа идёт первой при итерации.

`SstReader::get(key, max_offset)` использует `lower_bound` для поиска записей с нужным ключом, затем возвращает первую с `offset ≤ max_offset`.

**MergeIterator**:  расширен двумя режимами: `deduplicate=true` (стандартный - сливает дубликаты, оставляет highest offset) и `deduplicate=false` (для compaction при наличии snapshot'ов - сохраняет все версии, но дропает записи ниже `min_live_offset`).

## Упрощения

`BasonLevel` зависит почти от всех предыдущих заданий, поэтому все зависимости я мало-мальски имплементировал для демонстрации и работоспособности тестов, но их имплементации насколько у меня получалось **упрощены** засчет **исключения компонет**/**неэффективных реализаций**.

Список намеренных упрощений:

| Задание | Упрощения |
| - | - |
| Assignment 1: BASON Codec | Имплементировал только `BasonRecord` и примитивные `encoder`/`decoder`.<br>Не делал `RON64`, `Strictness Validator`, `Flat/Nested Conversion`. |
| Assignment 2: Write-Ahead Log | `Writer` умеет потоково добавлять записи, синкать запись на диск, сохранять чекпонит, ротировать сегментные файлы.<br>Нет `BLAKE3` хэшей и их сравнения, чекпоинт просто помечается символом `H`.<br>`Reader` умеет восстанавливаться по последнему чекпонинту и сканить записи.<br>Нет WAL GC, то есть функции `wal_truncate_before()`. |
| Assignment 3: SST Files (BASON Tables) | `SstWriter` не потоковый - он накапливает все записи в памяти, и пишет в файл только на `finish`.<br>Snappy/zstd сжатие не реализовано, данные хранятся без сжатия.<br>Bloom Filter не строится и не записывается.<br>`SstReader` также не потоковый - вычитывает файл целиком, и отвечает на запросы из RAM, а не с диска.<br>Bloom Filter не читается, поскольку не записан.<br>Индексы также не используются для лукапа с диска. |
| Assignment 4: Memtable and Merge Iterator | `Memtable` использует `std::map` вместо Skip List, все операции под мьютексом. |
|

## Сборка и тестирование

Проект написан на **C++**, использует систему сборки **Bazel**.

### Требования

* **C++ Standard:** C++23
* **Компилятор:** Clang 18 (со стандартной библиотекой `libc++`)
* **Система сборки:** [Bazel](https://bazel.build/) 8.0.0
* **Статический анализ и форматирование:** clang-tidy / clang-format

Детали конфигурации сборки см. в [.bazelrc](./.bazelrc)

### Сборка
```bash
# Build everything
bazel build //...

# Build level component
bazel build //level/...

# Build specific component
bazel build //level:<component name>
```

### Тестирование

Тесты написаны с использованием библиотеки [Google Test](https://github.com/google/googletest)

```bash
# Run all level tests
bazel test //level/tests/...

# Run specific component tests
bazel test //codec/tests:record_test
bazel test //memtable/tests:memtable_test
bazel test //memtable/tests:merge_iterator_test
bazel test //sst/tests:sst_test
bazel test //wal/tests:...

# Run all tests
bazel test //...
```

### Статический анализ кода

```bash
# Run clang-tidy checks
bazel build --config=clang-tidy //...
```

### Форматирование

```bash
# Run clang-format checks
bazel build --config=clang //...

# Run clang-format code formatting
bazel build --config=clang-format-fix //...
```

## Пример

```bash
bazel run //examples:example
```

Исходный код: [`examples/example.cpp`](./examples/example.cpp)

```cpp
auto db = BasonLevel::open({.dir = "/tmp/db", .memtable_size = 4096});

db->put("key1", record);  // запись
auto val = db->get("key1");  // чтение
db->del("key1");  // удаление

auto iter = db->scan("a", "z");  // scan
while (iter->valid()) { iter->next(); }

auto snapshot = db->snapshot();  // snapshot
db->put("key1", new_record);
auto old = db->get("key1", ReadOptions{.snapshot = snapshot.get()});

db->compact_level(0);  // compaction
auto m = db->metrics();  // метрики

db->close();  // закрытие

auto db2 = BasonLevel::open({.dir = "/tmp/db"});   // восстановление
```
