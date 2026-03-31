# Merkle Hashing for Data (Base and Branches)

## О чём проект

Реализация Merkle-дерева на Go 1.21 для демонстрации темы **Merkle hashing for data (base and branches)**.
Проект строит Merkle-дерево из набора данных, получает корень (`root`), умеет формировать и проверять Merkle proof для доказательства принадлежности элемента.

## Что уже решаем

- входные данные из JSON-файла;
- хэширование только через `SHA-256`;
- поддержка базы (`base`) и ветвей (`branches`) дерева;
- построение дерева и получение `root`;
- получение `proof` для произвольного листа;
- верификация `proof`;
- обновление листа и пересчёт цепочки вверх к корню;
- юнит-тесты.

## Формат ввода (JSON)

Пример файла `data.json`:

```json
{
  "items": [
    "block-0",
    "block-1",
    "block-2",
    "block-3"
  ]
}
```

В коде можно хранить строки как `[]byte`, либо расширить типы на произвольные JSON-объекты.

## Проектная структура

- `internal/merkle` — библиотечная реализация Merkle-дерева;
- `cmd` (опционально) — можно добавить CLI позже;
- `README.md` — эта документация;
- `internal/merkle/*_test.go` — тесты.

## Базовые термины

- **Base (leaf)** — хэш исходных данных:

`leaf = SHA-256(data)`

- **Branch** — внутренний узел из двух детей:

`parent = SHA-256(left.Hash || right.Hash)`

- **Root** — последний оставшийся узел после сворачивания уровней.

## Нечётное число узлов: важный выбор

Есть два распространённых подхода:

1. **Дублирование последнего (`duplicate last`)**
   - при нечётном количестве узлов на уровне последний узел дублируется;
   - ветви всегда идут попарно.

   Пример:
   - листья: `A, B, C`
   - уровни: `(A,B)->AB`, `C` дублируется -> `(C,C)->CC`
   - root из `AB` и `CC`

   **Плюсы**: детерминированный размер proof, удобная структура.

   **Минусы**: семантически это изменение входа в логике построения (хотя для Merkle-дерева это классика).

2. **Подъём последнего узла (`carry up`)**
   - последний узел просто переносится на уровень выше без создания пары.

   Пример:
   - листья: `A, B, C`
   - уровень: `(A,B)->AB`, `C` без пары переносится вверх;
   - root из `AB` и `C`.

   **Плюсы**: ближе к “реальному” дереву с разной степенью узлов.

   **Минусы**: длина proof может отличаться для разных листьев, нужно аккуратнее с верификацией.

Для домашней работы обычно берут **дублирование последнего** как наиболее стандартное и удобное поведение.

## Предлагаемое API

- `type MerkleTree struct { ... }`
- `type ProofStep struct {`
  `SiblingHash []byte`
  `IsLeft      bool`
  `}`
- `func NewMerkleTreeFromJSON(path string) (*MerkleTree, error)`
- `func NewMerkleTree(data [][]byte) *MerkleTree`
- `func (t *MerkleTree) Root() []byte`
- `func (t *MerkleTree) GetProof(index int) ([]ProofStep, error)`
- `func (t *MerkleTree) Verify(data []byte, proof []ProofStep, root []byte) bool`
- `func (t *MerkleTree) UpdateLeaf(index int, newData []byte) error`

## Кодирование хэшей

По умолчанию для чтения/вывода в отчетах и тестах удобно использовать **hex**.
При желании можно заменить на base64 в нескольких местах:

- формат сериализации `[]byte` в строку;
- логирование `root` и `proof`.

## Сложность

- Построение дерева: `O(n)`
- Проверка proof: `O(log n)`
- Обновление листа: `O(log n)` для пересчёта пути к корню

## Примеры сценариев

- Построение дерева из `data.json`
- Получение `proof` для элемента `i`
- Проверка proof для:
  - валидного элемента (ожидаем `true`);
  - изменённого элемента (ожидаем `false`).

## Что проверить в тестах

- пустой ввод (ожидаемая ошибка валидации);
- один элемент;
- два элемента;
- нечётный размер входа;
- все элементы уникальны/повторяются;
- корректная и невалидная верификация;
- после `UpdateLeaf` изменился только нужный путь к `root`.

## Идея для отчёта

В отчёте для курса хорошо показать:
- как формируются base и branches;
- почему меняется корень при изменении любого листа;
- зачем нужен proof;
- почему одинаковый набор данных всегда даёт тот же root.

## Как проверить на своей машине

```bash
cd "/Users/andrey300902/Desktop/merkle hashing"
go test ./...
```

### Ключевые команды CLI

```bash
cd "/Users/andrey300902/Desktop/merkle hashing"
go run ./cmd/merkle build -json example_data.json

go run ./cmd/merkle proof -json example_data.json -index 1

go run ./cmd/merkle verify -json example_data.json -index 1 -data block-1

go run ./cmd/merkle verify -json example_data.json -index 1 -data bad-value

go run ./cmd/merkle update -json example_data.json -index 1 -data "block-1-updated"
```

### Пример verify-proof

```bash
cd "/Users/andrey300902/Desktop/merkle hashing"
ROOT=$(go run ./cmd/merkle build -json example_data.json)
go run ./cmd/merkle proof -json example_data.json -index 1 > /tmp/merkle_proof.json
go run ./cmd/merkle verify-proof -root "$ROOT" -proof /tmp/merkle_proof.json -data block-1
```

Параметр `-proof` для `verify-proof` может быть JSON-массивом:

```json
[{"sibling":"<hex>", "is_left":true}]
```
или объектом с полем `proof` (результат команды `proof`).

Если нужна готовая входная выборка:
- `example_data.json` использует формат `{"items":[...]}`
- его можно читать через `NewMerkleTreeFromJSON(".../example_data.json")`.

## Примечание для отчёта

В задании используется подход `duplicate last` при нечётном числе узлов на уровне.
Это даёт фиксированную глубину proof и более удобную проверяемость.
