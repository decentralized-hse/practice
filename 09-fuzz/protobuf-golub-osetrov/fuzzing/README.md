### Build
```shell
$ clang++ -std=c++20 -g -fsanitize=fuzzer,address -o fuzz fuzz.cpp ../protobuf-golub-osetrov.cpp ../protobuf-golub-osetrov.pb.cc ../utils.cpp -lprotobuf
```

### Run
```shell
$ mkdir MY_CORPUS
$ ./fuzz MY_CORPUS/ valid-data/
```
