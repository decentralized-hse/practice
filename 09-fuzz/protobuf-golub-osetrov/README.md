## 09-fuzz homework

### Сборка бинарника
Чтобы собрать:
```shell
$ protoc -I=. --cpp_out=. ./protobuf-golub-osetrov.proto
$ mkdir build; cd build
$ cmake ..; cmake --build .
```

Запустить
```shell
$ ./build/protobuf-golub-osetrov ivanov.bin
$ ./build/protobuf-golub-osetrov ivanov.protobuf
```

### Фаззинг
Чтобы собрать и запустить фаззинг нужно выполнить:
```shell
$ cd fuzzing
$ mkdir build; cd build
$ cmake ..; cmake --build .
$ cd ../fuzz_dir
$ mkdir MY_CORPUS
$ ../build/fuzz MY_CORPUS/ valid_data/
```
