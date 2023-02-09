##  Задачи лекции 04 Форматы сериализации

Задача: написать преобразование из "квадратного" C-формата в ваш
и обратно, *с побитной точностью* (round-trip guarantee). Ваш:

 1. protobuf (protobuf)
 2. JSON (json)
 3. Cap'n proto (capnproto)
 4. Flat buffers (flat)
 5. Key: value (kv)
 6. sqlite (sqlite)
 7. XML (xml)
 8. sstable (sstable)

Оригинальный С формат (bin), little endian:

````c
    struct Student {
        // имя может быть и короче 32 байт, тогда в хвосте 000
        // имя - валидный UTF-8
        char    name[32];
        // ASCII [\w]+
        char    login[16];
        char    group[8];
        // 0/1, фактически bool
        uint8_t practice[8];
        struct {
            // URL
            char    repo[63];
            uint8_t mark;
        } project;
        // 32 bit IEEE 754 float
        float   mark; 
    }
````

```
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         name                                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           login               |     group     |   practice    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            repo                               |
|                                                             |m|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| mark  |
+-+-+-+-+
```

Как обычно, решение должно быть в файле с названием, напр.
`protobuf-ivanov.go`, единственный параметр - название файла,
либо с данными в формате C struct, либо в вашем формате.
В реализации вашего формата у вас есть некоторая свобода,
единственно оно должно быть идиоматично для формата, то
при обратном преобразовании, bin файл должен совпадать
побитно. В bin файле может быть несколько записей подряд.

````
    $ sha256sum data/students.bin
    0c016dc4ca1a8b24b34daa6c05472d16b401fd3c0c5417da18e9810b136704a6 students.bin
    $ ./protobuf-ivanov data/students.bin
    Reading binary student data from data/students.bin...
    3 students read...
    written to data/students.protobuf
    $ rm -f data/students.bin
    $ ./protobuf-ivanov data/students.protobuf
    Reading protobuf student data from data/students.protobuf...
    3 students read...
    written to data/students.bin
    $ sha256sum data/students.bin
    0c016dc4ca1a8b24b34daa6c05472d16b401fd3c0c5417da18e9810b136704a6 students.bin
````

Также, см пример `bin-gritzko.c` и `ivanov.bin`. 
