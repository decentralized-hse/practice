# key-value Стрыгин Александр

Написал тест, его можно запустить как
```bash
cd test
gcc create_test_file.c 
./a.out
cp test.bin test_true.bin
cd ..

cargo run test/test.bin
cargo run test/test.kv

alex$ shasum test/test_true.bin 
a49706775de5fc97053f6700525eadac656d7265  test/test_true.bin
alex$ shasum test/test.bin 
a49706775de5fc97053f6700525eadac656d7265  test/test.bin

```
