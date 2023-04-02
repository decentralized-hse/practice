### 04-formats homework

Чтобы собрать:
```shell
protoc -I=. --cpp_out=. ./protobuf-golub.proto
g++ -std=c++17 -o protobuf-golub protobuf-golub.cpp protobuf-golub.pb.cc `pkg-config --cflags --libs protobuf`
```

Запустить
```shell
./protobuf-golub ivanov.bin
```