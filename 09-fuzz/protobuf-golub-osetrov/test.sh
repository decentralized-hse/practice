#!/bin/bash
set -ex

rm test.bin test-initial.bin test.protobuf
cp $1 ./test.bin
./protobuf-golub-osetrov test.bin
mv test.bin test-initial.bin
./protobuf-golub-osetrov test.protobuf
diff test.bin test-initial.bin
