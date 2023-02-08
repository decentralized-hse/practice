##  Задачи лекции 04 Форматы сериализации

Задача: написать преобразование из "квадратного" C-формата в ваш
и обратно, с побитной точностью (round-trip guarantee). Ваш:

 1. protobuf
 2. JSON
 3. Cap'n proto
 4. Flat buffers
 5. Key: value
 6. sqlite
 7. XML

Оригинальный С формат (little endian):

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
