# BitTorrent DHT Scanner - сбор и обработка данных

## Авторы: 
Окунев Данила, Лавицкая Александра, Яппаров Эрик

Проект предназначен для сканирования BitTorrent DHT-сети с целью сбора IP-адресов участников заданной раздачи (по magnet-ссылке или info_hash) и всех промежуточных DHT-узлов. Собранные данные проходят очистку, валидацию, обогащение географическими данными и информацией о провайдерах, после чего визуализируются.

## Использование

Весь пайплайн состоит из последовательного запуска четырёх программ:

```bash
# 1. Сканирование DHT
./dht_scanner "<magnet-ссылка_или_info_hash>" 300 peers.bin

# 2. Фильтрация IP
./filter_ips peers.bin | sort | uniq > clean_ips.txt

# 3. Обогащение данными
./enrich_geo clean_ips.txt > enriched.jsonl

# 4. Статистика и визуализация
./statistics < enriched.jsonl
```

## Пример

Для примера использован публичный торрент с Ubuntu:

```bash
./dht_scanner "magnet:?xt=urn:btih:08ada5a7a6183aae1e09d831df6748d566095a10" 300 peers.bin
# Пример вывода во время работы:
#   [48/300s] IP: 280563 | Нод: 330939 | Ответов: 194894 | ~5845 IP/сек

./filter_ips peers.bin | sort | uniq > clean_ips.txt
# Размер файла: 1738220 байт, ожидается 434555 IP
#
# Обработка IP...
#   Обработано 10000/434555...
#   Обработано 20000/434555...
#
# Найдено уникальных публичных IP: 404988
#   Всего IP в файле: 434555
#   Отсеяно: 29567 (приватные/дубликаты)

./enrich_geo clean_ips.txt > enriched.jsonl

head -3 enriched.jsonl
```
```json
{"ip":"88.185.238.156","country":"Франция","country_code":"FR","city":"Париж","latitude":48.8566,"longitude":2.3522,"asn":12322,"aso":"Free SAS"}
{"ip":"31.200.249.233","country":"Россия","country_code":"RU","city":"Москва","latitude":55.7558,"longitude":37.6173,"asn":25515,"aso":"JSC ER-Telecom Holding"}
{"ip":"146.70.137.18","country":"США","country_code":"US","city":"Нью-Йорк","latitude":40.7128,"longitude":-74.0060,"asn":36352,"aso":"AS-COLOCROSSING"}
```

## Алгоритм работы

1. **`dht_scanner`** - подключается к DHT-сети через bootstrap-узлы. Обходит сеть, отправляя запросы `get_peers` (для поиска пиров заданного `info_hash`) и `find_node` (для поиска новых DHT-узлов). Все найденные уникальные IP-адреса (как пиров, так и узлов) сохраняются в бинарный файл.
2. **`filter_ips`** - фильтрует только публичные IPv4-адреса, исключая приватные диапазоны (10.0.0.0/8, 192.168.0.0/16 и др.) и удаляя дубликаты.
3. **`enrich_geo`** - для каждого IP определяет страну, город, координаты (через GeoLite2 City) и провайдера (через GeoLite2 ASN). Результат сохраняется в формате JSON Lines.
4. **`statistics`** - подсчитывает статистику и создаёт html-отчёт: топ стран и провайдеров, общее количество IP.

## Требования

- Компилятор C (gcc/clang)
- Библиотека `libmaxminddb` (для `enrich_geo`)
- Базы GeoLite2 City и ASN (скачиваются отдельно с сайта MaxMind)
- gnuplot для визуализации

## Сборка

```bash
gcc -O2 -Wall -Wextra -std=c11 -o dht_scanner dht_scanner.c
gcc -O2 -Wall -Wextra -std=c11 -o filter_ips filter_ips.c
gcc -O2 -Wall -Wextra -std=c11 -o enrich_geo enrich_geo.c -lmaxminddb
gcc -O2 -Wall -Wextra -std=c11 -o statistics statistics.c
```
## Примечания
Сканер сохраняет данные в бинарном формате для компактности и скорости. Утилита `filter_ips` выполняет конвертацию в текстовый формат и фильтрацию приватных адресов
