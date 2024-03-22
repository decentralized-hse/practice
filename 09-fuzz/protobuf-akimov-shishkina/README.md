## [09-fuzz] Hometask

#### Запуск:
Сгенерировать структуры по proto файлу:
```
$ protoc -I=protobuf --go_out=./protobuf/gen student.proto
```

Запустить программу:
```
$ go run main.go ivanov.bin
$ go run main.go ivanov.protobuf
```

Запустить фаззинг-тест (остановила после 40 минут):
```
$ go test -fuzz=Fuzz ./test
```

#### Исправления
За основу было взято решение [04-formats/protobuf_akimov.go](https://github.com/decentralized-hse/practice/blob/main/04-formats/protobuf_akimov.go)

__Что было исправлено__:
1. Код разбила на функции, добавила везде обработку ошибок.
2. Добавила проверки на валидность данных (поскольку строка может содержать байты, не соответствующие UTF-8).
3. В решении не было proto файла, поэтому я написала свой, изменила некоторые типы и убрала костыли по типу: ```const size_one_student = 142```.