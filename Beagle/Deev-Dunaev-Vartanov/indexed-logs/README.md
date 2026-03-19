# Indexed Logging System on Beagle

Веб-приложение для хранения, индексирования и поиска логов поверх `Beagle` и его HTTP-интерфейса `be-srv`.

Проект рассматривает Beagle не как классическую SQL/NoSQL базу данных, а как versioned storage layer: логи и индексы живут как набор файлов, а приложение работает с ними через HTTP.

## Идея проекта

В проекте реализована индексированная система логирования со следующими свойствами:

- каждый лог хранится как отдельный JSON-файл в каталоге `logs/`
- пользовательский интерфейс написан на JavaScript и открывается через `be-srv`
- поиск работает не по полному сканированию всех записей, а через отдельные индексы
- индексы хранятся рядом с логами, в каталоге `indexes/`
- приложение использует только HTTP-интерфейс Beagle, без отдельного Node backend

Это соответствует формулировке задания: `indexed logging system (with search)`.

## Что умеет приложение

На текущий момент приложение умеет:

- открывать веб-интерфейс по адресу `/app/index.html`
- показывать все сохранённые логи
- добавлять новый лог через форму
- искать логи по `level`
- искать логи по `source`
- искать логи по дате
- искать логи по текстовому запросу
- комбинировать фильтры `level + source + date + term`
- пересобирать индексы из каталога `logs/`
- хранить поисковые параметры в URL, чтобы состояние поиска можно было открыть повторно

## Что именно хранится в проекте

### Формат лога

Каждый лог хранится как отдельный JSON-файл:

```json
{
  "id": "log-demo-auth-error",
  "timestamp": "2026-03-18T10:25:00.000Z",
  "source": "auth-service",
  "level": "ERROR",
  "message": "Token verification failed because JWT signature expired",
  "tags": ["auth", "jwt", "token"]
}
```


### Что такое `.be` и `.be-home`

- `.be` связывает текущую рабочую директорию с Beagle project
- `.be-home` используется как локальный home для `be` и `be-srv`, чтобы проект не зависел от глобального окружения пользователя

## Архитектура

Приложение состоит из трёх частей.

### 1. UI

Каталог [`app/`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/app) содержит фронтенд:

- [`index.html`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/app/index.html) — разметка интерфейса
- [`styles.css`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/app/styles.css) — стили
- [`app.js`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/app/app.js) — логика формы, отображения логов и поиска
- [`api.js`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/app/api.js) — HTTP-слой поверх `be-srv`
- [`indexing.js`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/app/indexing.js) — общая логика токенизации и построения индексов

### 2. Данные

Каталог [`logs/`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/logs) содержит сами лог-записи.

Каталог [`indexes/`](/Users/valerii/Desktop/HW/decentralized-hse/practice/Beagle/Deev-Dunaev-Vartanov/indexed-logs/indexes) содержит индексы и служебный `manifest.json`.

### 3. Storage layer

Рядом с проектом лежит исходный код Beagle в `../librdx`.

Важная идея проекта:

- `Beagle` отвечает за версионное файловое хранилище и HTTP-доступ
- `be-srv` отдаёт UI, логи и индексы по HTTP
- приложение не работает напрямую с RocksDB или внутренними структурами Beagle

## Как устроены индексы

Проект использует несколько типов индексов:

- `by-level` — индекс по уровню лога
- `by-source` — индекс по источнику
- `by-day` — индекс по дню
- `by-term` — inverted index по токенам из `message`, `source`, `level` и `tags`

### Почему в индексах есть и `.json`, и marker-файлы

В текущей сборке Beagle повторные `POST` в один и тот же JSON-массив работают нестабильно. Поэтому в проекте используется два представления индекса:

- `*.json` — человекочитаемые materialized buckets, полезны для просмотра и полной пересборки
- `bucket__log-id.txt` — marker-файлы для стабильного инкрементального обновления индекса при добавлении новых логов

Пример:

```text
indexes/by-level/WARN.json
indexes/by-level/WARN__log-demo-auth-warn.txt
indexes/by-level/WARN__log-demo-cache-warn.txt
indexes/by-level/WARN__log-demo-scheduler-warn.txt
```

Практически это означает следующее:

- при добавлении нового лога приложение создаёт сам лог в `logs/`
- затем создаёт marker-файлы в нужных индексных каталогах
- поиск читает marker-файлы как основной источник truth для live-обновлений
- `JSON`-индексы остаются как snapshot и как удобный артефакт для проверки структуры проекта

## Как работает поиск

Поиск в приложении идёт в два этапа.

### 1. Сужение кандидатов по индексам

Если пользователь указывает фильтры, приложение сначала получает кандидатов:

- по `level` из `indexes/by-level/`
- по `source` из `indexes/by-source/`
- по `date` из `indexes/by-day/`
- по токенам запроса из `indexes/by-term/`

Если фильтров несколько, используется пересечение наборов `id`.

### 2. Финальная проверка

После этого подгружаются только найденные кандидаты, и на них выполняется финальная проверка:

- совпадение по структурным фильтрам
- проверка того, что все токены из текстового запроса действительно присутствуют в логе

Такой подход проще и эффективнее, чем каждый раз читать и фильтровать все файлы из `logs/`.

## Демо-данные

В репозитории уже есть набор демонстрационных логов.

## Как запустить проект

### Предварительные условия

Нужно, чтобы рядом с `indexed-logs` лежал репозиторий `librdx` и был собран `be-srv`.

Если `be-srv` ещё не собран:

```bash
cd Beagle/Deev-Dunaev-Vartanov
cmake -S librdx -B librdx/build
cmake --build librdx/build --target be-srv
```

### Запуск сервера

Из каталога `indexed-logs`:

```bash
cd Beagle/Deev-Dunaev-Vartanov/indexed-logs
HOME=$PWD/.be-home ../librdx/build/be/be-srv 8080
```

### Поиск

В форме поиска можно использовать:

- `Text`
- `Level`
- `Source`
- `Date`

Фильтры можно комбинировать.

### Пересборка индексов

Если хочется восстановить индексное состояние только из логов, можно:

- нажать кнопку `Rebuild Indexes` в UI
- или выполнить локальный скрипт

```bash
cd Beagle/Deev-Dunaev-Vartanov/indexed-logs
node scripts/rebuild-indexes.mjs
```

После скрипта, если нужно обновить данные в самом Beagle-репозитории, их следует снова опубликовать через `be post`.


## Ограничения текущей версии

Сейчас проект ещё не делает некоторые вещи.

- Нет UI для удаления логов.
- Нет полноценного редактирования существующего лога.
- В проекте пока нет отдельного inverted index на уровне сложного full-text engine; поиск остаётся простым, но уже индексированным.

