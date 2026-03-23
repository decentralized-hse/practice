#include "wal.hpp"
#include <cassert>
#include <iostream>

int main() {
    system("rm -rf wal && mkdir wal");

    auto writer = WalWriter::open("wal");

    for (int i = 0; i < 1000; i++) {
        writer.append(BasonRecord("record_" + std::to_string(i)));
        writer.rotate(256);
    }

    writer.checkpoint();
    writer.sync();

    auto reader = WalReader::open("wal");
    reader.recover();

    auto it = reader.scan(0);

    int count = 0;
    while (it.valid()) {
        count++;
        it.next();
    }

    assert(count == 1000);

    std::cout << "OK rotate\n";
}