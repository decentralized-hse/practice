#include "wal.hpp"
#include <cassert>
#include <iostream>

int main() {
    system("rm -rf wal && mkdir wal");

    auto writer = WalWriter::open("wal");

    auto a = writer.append(BasonRecord::leaf_string("first"));
    auto b = writer.append(BasonRecord::leaf_string("second"));

    writer.checkpoint();
    writer.sync();

    auto reader = WalReader::open("wal");
    auto recovered = reader.recover();

    assert(recovered > 0);

    auto it = reader.scan(0);

    int count = 0;
    while (it.valid()) {
        count++;
        it.next();
    }

    assert(count == 2);

    std::cout << "OK basic\n";
}