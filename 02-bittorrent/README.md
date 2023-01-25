#   Задачи лекции 02 BitTorrent 

Для заданного файла с данными, дерево хэшей определяется, как 
двоичное, с размером куска 1KB и хэш функцией [SHA-256][h]. 
Для простоты, размер файла данных кратен 1KB. 
Раскладка дерева в файле хэшей: [in order][i], aka [bin numbers][b].
Хэш всего дерева, он же хэш файла, определяется, как SHA256 хэш 
*файла пиков*. Файл пиков содержит 32 ячейки, где в ячейке i либо
хэш пика на уровне i, либо нули, если пика такого пика нет.
В статье Russ Cox термина "peak" нет, там говорится hashes of
complete trees. В [RFC7574][r] также используется термин munro.
(Размер такого файла пиков 65x32 байт).
Подпись накладывается на хэш дерева по алгоритму [Ed25519][e].

 1. создать дерево хэшей для файла, по спеке, с bin раскладкой
    по RFC7574 `hashtree`
 2. положить пиковые хэши в отдельный файл `peaks`
    Хэши сохраняем [в hex][x], с переводом строки (65 байт на хэш,
    можно читать как текстом, так и по смещению).
 3. посчитать хэш файла, алгоритм SHA256, на bash `root`
    Сохранить в файл (65 байт).
 4. создать доказательство целостности для пакета данных `proof`
    Доказательство состоит из последовательности дядиных хэшей
    от 0 этажа вверх, в том же hex формате.
 5. проверить доказательство целостности для пакета `verify`
 6. подписать файл `sign`
 7. проверить подпись `check`

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

`verify` передаёт результат кодом возврата (0/не 0) и пишет на
экран.

[i]: https://research.swtch.com/tlog#appendix_b
[h]: https://libsodium.gitbook.io/doc/advanced/sha-2_hash_function
[e]: https://libsodium.gitbook.io/doc/public-key_cryptography/public-key_signatures
[b]: https://github.com/gritzko/binmap/blob/master/bin.h
[x]: https://doc.libsodium.org/helpers
[r]: https://www.rfc-editor.org/rfc/rfc7574.html#section-5.6.1
