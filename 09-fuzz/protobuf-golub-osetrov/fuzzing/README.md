### Build
```shell
$ mkdir build; cd build
$ cmake ..; cmake --build .
```

### Run
```shell
$ cd fuzz_dir
$ mkdir MY_CORPUS
$ ../build/fuzz MY_CORPUS/ valid-data/
```
