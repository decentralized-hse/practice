#include "wal.hpp"

#include <cassert>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    system("rm -rf wal && mkdir wal");

    for (int i = 0; i < 10000; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            auto writer = WalWriter::open("wal");

            for (int j = 0; j < 100; j++) {
                writer.append(BasonRecord::leaf_string("data_" + std::to_string(j)));

                if (j % 10 == 0) {
                    writer.checkpoint();
                }
            }

            writer.sync();
            exit(0);
        } else {
            usleep(rand() % 5000);

            kill(pid, SIGKILL);

            waitpid(pid, nullptr, 0);

            auto reader = WalReader::open("wal");
            reader.recover();

            auto it = reader.scan(0);

            uint64_t prev = 0;

            while (it.valid()) {
                auto off = it.offset();
                assert(off >= prev);
                prev = off;

                it.next();
            }
        }

        if (i % 1000 == 0) {
            std::cout << "Crash test iteration: " << i << "\n";
        }
    }

    std::cout << "OK crash test\n";
}