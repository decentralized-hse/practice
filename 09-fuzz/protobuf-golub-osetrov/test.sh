#!/bin/bash
set -ex

touch test.bin test-initial.bin test.protobuf
rm test.bin test-initial.bin test.protobuf
cp $1 ./test.bin
./build/protobuf-golub-osetrov test.bin
mv test.bin test-initial.bin
./build/protobuf-golub-osetrov test.protobuf
diff test.bin test-initial.bin
