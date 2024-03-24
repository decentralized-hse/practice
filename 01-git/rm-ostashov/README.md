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
Ожидаем 02553a5bca84d9e0167c8d159c1b171a126d8bbddc17198831a33a3de54648b0

## Удалить поддиректорию:
```bash
python3 ../rm.py dir c45a2ce369ad3b6b2f1463dded8215829f9ce53f210d16b2b8626b6c9d3fd845
```
Ожидаем 7aed1315129df81a348dfcc4a22db66a7d1924042d5a4633eb9ee952a76ee4fa

## Удалить файл:
```bash
python3 ../rm.py input c45a2ce369ad3b6b2f1463dded8215829f9ce53f210d16b2b8626b6c9d3fd845
```
Ожидаем e567a5ec0d998c25a39369dc97d403678d9a96e429d9d4e911b43a4f212e95e0

## Проверка .parent:
```bash
python3 ../rm.py input.txt 02553a5bca84d9e0167c8d159c1b171a126d8bbddc17198831a33a3de54648b0 
```
Ожидаем 63d54f7604d73de90dacfec2d180f2017793ed319c863ada14965cb32b4f47d3