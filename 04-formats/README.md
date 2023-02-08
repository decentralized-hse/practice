##  Задачи лекции 04 Форматы сериализации

Задача: написать преобразование из "квадратного" C-формата в ваш
и обратно, с побитной точностью (round-trip guarantee). Ваш:

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
        char    name[32];
        char    login[16];
        char    group[8];
        uint8_t practice[8];
        struct {
            char    repo[63];
            uint8_t mark;
        } project;
        float   mark; // 32 bit
    }
````

Как обычно, решение должно быть в файле с названием, напр.
`protobuf-ivanov.go`, единственный параметр - название файла с
данными в формате C struct, напр

````
    $ ./protobuf-ivanov data/ivanov.bin
    Reading binary student data from data/ivanov.bin...
    3 students read...
    written to data/ivanov.protobuf
    $ ./protobuf-ivanov data/ivanov.protobuf
    Reading protobuf student data from data/ivanov.protobuf...
    3 students read...
    written to data/ivanov.bin
````
