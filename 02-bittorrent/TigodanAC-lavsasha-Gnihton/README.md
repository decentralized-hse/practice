# BitTorrent DHT Scanner - сбор и обработка данных

## Авторы: 
Окунев Данила, Лавицкая Александра, Яппаров Эрик

Проект предназначен для сканирования BitTorrent DHT-сети с целью сбора IP-адресов участников заданной раздачи (по magnet-ссылке или info_hash), их последующей очистки, валидации, обогащения географическими данными и информацией о провайдерах, а также визуализации полученных результатов.

## Использование

Весь пайплайн состоит из последовательного запуска четырёх программ:

```bash
# 1. Сканирование DHT
./dht_scanner "<magnet-ссылка_или_info_hash>" 60 peers.txt

# 2. Фильтрация IP
./filter_ips < peers.txt | sort | uniq > peers_clean.txt

# 3. Обогащение данными
./enrich_geo peers_clean.txt > peers_enriched.jsonl

# 4. Статистика и визуализация
./statistics < peers_enriched.jsonl
```

## Пример

Для примера использован публичный торрент с Ubuntu:

```bash
./dht_scanner "magnet:?xt=urn:btih:08ada5a7a6183aae1e09d831df6748d566095a10" 60 peers.txt
./filter_ips < peers.txt | sort | uniq > peers_clean.txt
./enrich_geo peers_clean.txt > peers_enriched.jsonl

head -3 peers_enriched.jsonl
```
```json
{"ip":"88.185.238.156","country":"Франция","country_code":"FR","city":"Париж","latitude":48.8566,"longitude":2.3522,"asn":12322,"aso":"Free SAS"}
{"ip":"31.200.249.233","country":"Россия","country_code":"RU","city":"Москва","latitude":55.7558,"longitude":37.6173,"asn":25515,"aso":"JSC ER-Telecom Holding"}
{"ip":"146.70.137.18","country":"США","country_code":"US","city":"Нью-Йорк","latitude":40.7128,"longitude":-74.0060,"asn":36352,"aso":"AS-COLOCROSSING"}
```

## Алгоритм работы

1. **`dht_scanner`** - подключается к DHT-сети BitTorrent, находит узлы, близкие к целевому `info_hash`, собирает IP-адреса пиров через `get_peers` запросы.
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

