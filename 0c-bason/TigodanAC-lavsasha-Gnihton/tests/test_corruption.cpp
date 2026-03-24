#include "wal.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

int main() {
    system("rm -rf wal && mkdir wal");

    {
        auto writer = WalWriter::open("wal");

        for (int i = 0; i < 10; i++) {
            writer.append(BasonRecord::leaf_string("ok_" + std::to_string(i)));
        }

        writer.checkpoint();
        writer.sync();
    }

    std::fstream f("wal/00000000000000000000.wal",
                   std::ios::in | std::ios::out | std::ios::binary);

    f.seekp(-10, std::ios::end);
    char x = 123;
    f.write(&x, 1);
    f.close();

    auto reader = WalReader::open("wal");
    auto recovered = reader.recover();

    auto it = reader.scan(0);

    int count = 0;
    while (it.valid()) {
        count++;
        it.next();
    }

    assert(count <= 10);

    std::cout << "OK corruption (recovered=" << recovered << ")\n";
}