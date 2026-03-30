#pragma once

#include "wal/iterator.hpp"

#include <cstdint>
#include <filesystem>

namespace bason_db {

    class WalReader {
    public:
        // Open an existing WAL directory for reading.
        static WalReader open(const std::filesystem::path& dir);

        // Recover after a crash. Returns the offset just past the last
        // valid hash checkpoint. Records beyond this point are discarded
        // (the segment file is truncated).
        uint64_t recover();

        // Create an iterator starting from a given global offset. Used
        // for replay during recovery, replication, and reads.
        WalIterator scan(uint64_t from_offset);

    private:
        std::filesystem::path dir_;
    };

} // namespace bason_db
