## [09-fuzz] Nardzhiev x Vernigor

#### 
Первоначальная реализация: [04-formats/protobuf-vernigor](https://github.com/decentralized-hse/practice/tree/main/04-formats/protobuf-vernigor)

__Что поменялось__:
1. Добавлен валидация данных на возможность декодирования байотов.
2. Добавлены обработчики исключения для завершения программы согласно условию.
3. Произведен рефакторинг кода, функции были разнесены по файлам для повыешения читаемости.
4. Добавлен фаззер на основе atheris. Проведен фаззинг (в течение 45 минут).
5. Отказался от константного размера pb объекта.


#### Перед началом:
Сгенерируйте pb2 по student.proto:
```
$ protoc -I=./protobuf_vernigor --python_out=./protobuf_vernigor ./protobuf_vernigor/student.proto
```

#### Запуск программы:
```
$ python3 protobuf_vernigor/protobuf_vernigor.py  test_data/test_input.bin
$ python3 protobuf_vernigor/protobuf_vernigor.py  test_data/test_input.protobuf
```

#### Запуск фаззера:
```
$ python3 protobuf_vernigor/fuzz.py
```
