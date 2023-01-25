#   Задачи лекции 02 BitTorrent 

Для заданного файла с данными, дерево хэшей определяется, как 
двоичное, с размером блока 1KB и хэш функцией [SHA-256][h]. 
Для простоты, размер файла данных кратен 1KB (нет незавершенного блока). 
Раскладка дерева в файле хэшей: [in order][i], aka [bin numbers][b].

Хэш всего дерева, он же хэш файла, определяется, как SHA256 хэш 
*файла пиков*. Файл пиков содержит 32 ячейки, где в ячейке i либо
хэш пика на уровне i, либо нули, если пика такого пика нет.
В статье Russ Cox термина "peak" нет, там говорится hashes of
complete trees. В [RFC7574][r] также используется термин munro.
(Размер такого файла пиков 65x32 байт).

Подпись накладывается на хэш дерева по алгоритму [Ed25519][e].

 1. создать дерево хэшей для файла, по спеке, с bin раскладкой
    по RFC7574 `*.hashtree`
    Хэши сохраняем [в hex][x], с переводом строки (65 байт на хэш,
    можно читать как текстом, так и по смещению).
 2. положить пиковые хэши в отдельный файл `*.peaks`, в том же
    формате (размер файла пиков 65*32=2080 байт).
 4. посчитать хэш файла, алгоритм SHA256, на bash
    Сохранить в файл `*.root` (65 байт).
 4. создать доказательство целостности для блока данных `*.proof`
    Доказательство состоит из последовательности дядиных хэшей
    от 0 этажа вверх, в том же hex формате. Доказательство позволяет
    проверить блок данных об его пиковый хэш.
 5. проверить доказательство целостности для блока `verify`
    Тут мы должны проверить, что пики соответствуют хэшу файла,
    а дядины хэши позволят проверить, что блок соответствует
    своему пику.
 7. подписать файл `sign` (файлы ключей `.pub` `.sec`, подпись
    в файле `*.sign`)
 8. проверить подпись `check`.

Результаты складывать в файлы с соответствующим расширением,
например `datafile.hashtree`, `datafile.peaks`, `datafile.sign`.
Исходные данные брать так же, из файлов. При вызове программы
первый аргумент всегда - название исходного файла. Пример:

````
    $ python peaks-ivanov.py pikachu.mov
    reading pikachu.mov.hashtree...
    putting the peaks into pikachu.mov.peaks...
    all done!
    $ bash root-petrova.sh pikachu.mov
    hashing pikachu.mov.peaks
    all done!
    $ clang++ proof-sidorov.cpp -lsodium -o proof-sidorov
    $ ./proof-sidorov pikachu.mov 18
    reading pikachu.mov.hashtree...
    putting the proof into pikachu.mov.18.proof...
    all done!
````

`verify`, `check` передают результат кодом возврата (0/не 0) и
пишут на экран.

[i]: https://research.swtch.com/tlog#appendix_b
[h]: https://libsodium.gitbook.io/doc/advanced/sha-2_hash_function
[e]: https://libsodium.gitbook.io/doc/public-key_cryptography/public-key_signatures
[b]: https://github.com/gritzko/binmap/blob/master/bin.h
[x]: https://doc.libsodium.org/helpers
[r]: https://www.rfc-editor.org/rfc/rfc7574.html#section-5.6.1
