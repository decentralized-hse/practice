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
Ожидаем 4cfde93628f3f4f39ac5dd7648eb08753d43c4977f20c5761525acb485e88637

## Удалить поддиректорию:
```bash
python3 ../rm.py dir c45a2ce369ad3b6b2f1463dded8215829f9ce53f210d16b2b8626b6c9d3fd845
```
Ожидаем baaac770bd36b8f73206f86776ce924b9448d2be6c8c3fdb7ea71728a6e3148f

## Удалить файл:
```bash
python3 ../rm.py input c45a2ce369ad3b6b2f1463dded8215829f9ce53f210d16b2b8626b6c9d3fd845
```
Ожидаем 88d9981394eb2339da18cb57fcfaa033624f99b62e3cb50f496541716bcd5006

## Проверка .parent:
```bash
python3 ../rm.py input.txt 4cfde93628f3f4f39ac5dd7648eb08753d43c4977f20c5761525acb485e88637 
```
Ожидаем 6172510f032b6ce56f9272217910cd3c04624e38c9aaaab71bec618e26d7ad83