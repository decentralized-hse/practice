#!/bin/bash
set -e

INFO_HASH="${1:-08ada5a7a6183aae1e09d831df6748d566095a10}"
DURATION="${2:-300}"
OUTPUT_DIR="/output"

mkdir -p "$OUTPUT_DIR/plots"

echo "=== 1/4: Сканирование DHT ==="
./dht_scanner "$INFO_HASH" "$DURATION" "$OUTPUT_DIR/peers.bin"

echo ""
echo "=== 2/4: Фильтрация IP ==="
./filter_ips "$OUTPUT_DIR/peers.bin" | sort -u > "$OUTPUT_DIR/clean_ips.txt"
echo "Сохранено: $OUTPUT_DIR/clean_ips.txt ($(wc -l < "$OUTPUT_DIR/clean_ips.txt") IP)"

echo ""
echo "=== 3/4: Обогащение геоданными ==="
if [ -f /app/geodb/GeoLite2-City.mmdb ] && [ -f /app/geodb/GeoLite2-ASN.mmdb ]; then
    ln -sf /app/geodb/GeoLite2-City.mmdb /app/GeoLite2-City.mmdb
    ln -sf /app/geodb/GeoLite2-ASN.mmdb /app/GeoLite2-ASN.mmdb
    ./enrich_geo "$OUTPUT_DIR/clean_ips.txt" > "$OUTPUT_DIR/enriched.jsonl"
    echo "Сохранено: $OUTPUT_DIR/enriched.jsonl ($(wc -l < "$OUTPUT_DIR/enriched.jsonl") записей)"

    echo ""
    echo "=== 4/4: Статистика и визуализация ==="
    cd "$OUTPUT_DIR"
    /app/statistics < enriched.jsonl
    echo ""
    echo "Результаты в $OUTPUT_DIR/plots/"
else
    echo "ПРОПУЩЕНО: GeoLite2 базы не найдены."
    echo "Положите GeoLite2-City.mmdb и GeoLite2-ASN.mmdb в директорию geodb/"
    echo "Шаги 3 и 4 пропущены. Результат сканирования: $OUTPUT_DIR/peers.bin и $OUTPUT_DIR/clean_ips.txt"
fi
