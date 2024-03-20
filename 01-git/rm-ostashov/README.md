# Как протестировать:

## Подготовка
```bash
pip install requirements.txt
cd test
```

## Удалить файл из поддиректории
```bash
python3 ../rm.py dir/input c45a2ce369ad3b6b2f1463dded8215829f9ce53f210d16b2b8626b6c9d3fd845
```
Ожидаем b9b29ea7cd229cc689a3543832d68b918dc30037bfcab5b4d95c9282bee070d5

## Удалить поддиректорию:
```bash
python3 ../rm.py dir c45a2ce369ad3b6b2f1463dded8215829f9ce53f210d16b2b8626b6c9d3fd845
```
Ожидаем df6c008b2815f5b04393a73f0acb35102eaa0c9f963832d86ac7084c2a1eba6e

## Удалить файл:
```bash
python3 ../rm.py input c45a2ce369ad3b6b2f1463dded8215829f9ce53f210d16b2b8626b6c9d3fd845
```
Ожидаем 35637537328b2f95f9fcdf4f9f12bc17925172155ef062172d6590f3e1901144
