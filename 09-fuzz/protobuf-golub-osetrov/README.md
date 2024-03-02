## 09-fuzz homework

### Сборка бинарника
Чтобы собрать:
```shell
$ protoc -I=. --cpp_out=. ./protobuf-golub-osetrov.proto
$ clang++ -std=c++20 -o protobuf-golub-osetrov main.cpp protobuf-golub-osetrov.cpp pro
tobuf-golub-osetrov.pb.cc utils.cpp -lprotobuf
```

Запустить
```shell
$ ./protobuf-golub-osetrov ivanov.bin
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
