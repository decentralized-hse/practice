#include "wal.hpp"
#include <chrono>
#include <iostream>

int main() {
    system("rm -rf wal && mkdir wal");

    const int N = 2000000;

    auto start = std::chrono::high_resolution_clock::now();

    auto writer = WalWriter::open("wal");

    for (int i = 0; i < N; i++) {
        writer.append_buffered(BasonRecord::leaf_string(std::string(100, 'x')));
        if (i % 1000 == 0) {
            writer.checkpoint();
            writer.sync_periodic();
        }
    }

    writer.checkpoint();
    writer.sync_periodic();

    auto end = std::chrono::high_resolution_clock::now();

    double sec = std::chrono::duration<double>(end - start).count();

    double mb = (N * 100.0) / (1024 * 1024);

    std::cout << "Write throughput: " << (mb / sec) << " MB/s\n";

    auto rstart = std::chrono::high_resolution_clock::now();

    auto reader = WalReader::open("wal");
    reader.recover();

    auto rend = std::chrono::high_resolution_clock::now();

    double rsec = std::chrono::duration<double>(rend - rstart).count();

    std::cout << "Recovery speed: " << (mb / rsec) << " MB/s\n";
}