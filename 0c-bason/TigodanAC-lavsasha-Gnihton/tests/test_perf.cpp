#include "wal.hpp"
#include <chrono>
#include <iostream>

int main() {
    system("rm -rf wal && mkdir wal");

    const int N = 2000000;
    // Один и тот же payload: в старом виде в цикле создавалось 2M копий std::string(100,'x') —
    // это давало лишние аллокации и шум в цифре «write throughput» (измеряется полезные 100 байт/запись).
    BasonRecord rec;
    rec.type = BasonType::String;
    rec.value.assign(100, 'x');

    auto start = std::chrono::high_resolution_clock::now();

    auto writer = WalWriter::open("wal");

    // Реже checkpoint: каждые 1000 итераций — тысячи финализаций BLAKE3 на прогон;
    // для оценки скорости записи достаточно редких границ транзакции.
    constexpr int kCheckpointEvery = 65536;
    for (int i = 0; i < N; i++) {
        writer.append_buffered(rec);
        if (i % kCheckpointEvery == 0) {
            writer.checkpoint();
        }
    }

    writer.checkpoint();
    // Один fsync в конце: типичный сценарий «дожали батч — сбросили на диск».
    // sync_periodic() в цикле сильно режет MB/s на микробенчмарке при включённом fsync.
    writer.sync();

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