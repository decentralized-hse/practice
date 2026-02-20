# dmod — децентрализованные submodules

Основная идея: заменить URL-ссылку на внешний репозиторий хэшем его содержимого,
а центральный сервер — распределённой сетью узлов (DHT).

---

## Проблема

Стандартные git submodules хранят зависимость так:
```ini
[submodule "libs/ui-lib"]
    url  = https://github.com/team/ui-lib
    path = libs/ui-lib
```

Возможные проблемы:

**Тихое расхождение состояний.** `git pull` обновляет только указатель на коммит
в индексе, но не содержимое папки submodule на диске. Чтобы обновить содержимое,
нужно отдельно запустить `git submodule update`. Если забыть — разные разработчики
на одном коммите будут компилировать с разными версиями зависимости.

**Link rot.** Зависимость хранится как URL — адрес места. Если репозиторий
переехал, удален или хостинг недоступен — зависимость перестает резолвиться.


---

## Решение

Вместо URL  — **CID** (Content Identifier): SHA-256 хэш содержимого.
Вместо центрального сервера — **DHT** (Distributed Hash Table): сеть узлов,
каждый из которых хранит часть контента.
```ini
[submodule "libs/ui-lib"]
    path = libs/ui-lib
    cid  = ee6c94ae53de8de0...
```


**Изменения видны в git diff.** Файл `.decentralized_modules` трекается Git
как обычный текстовый файл. Когда кто-то обновляет зависимость, в `git diff`
появляется обычная строка:
```diff
-    cid  = ee6c94ae53de8de0...
+    cid  = b84384c0544f33cf...
```

---

## Как это работает вместе с Git

dmod не заменяет Git — он работает рядом. Git отвечает за историю проекта,
dmod отвечает за зависимости. Их точка соприкосновения — файл
`.decentralized_modules`, который живёт в репозитории как обычный файл.
```bash
# Добавить зависимость
dmod_add(&dht, &modules, "ui-lib", "libs/ui-lib", content, len);
git add .decentralized_modules
git commit -m "add ui-lib"
git push

# После git pull — получить зависимости
git pull
dmod_update(&dht, &modules);

# Обновить версию зависимости
dmod_publish(&dht, &modules, "ui-lib", new_content, new_len);
git add .decentralized_modules
git commit -m "bump ui-lib"
git push
```

---

## Структура кода

`dmod.h` — интерфейс: структуры и объявления функций.

`dmod.c` — реализация:
- SHA-256 без внешних зависимостей
- DHT: публикация с репликацией, поиск с верификацией хэша
- Чтение и запись `.decentralized_modules`
- Команды `dmod_add`, `dmod_update`, `dmod_publish`

Реализация симулирует DHT в одном процессе.

