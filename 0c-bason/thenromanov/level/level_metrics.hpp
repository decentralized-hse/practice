#pragma once

#include "level/constants.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace bason_db {

    struct LevelMetrics {
        std::array<size_t, kTotalLevels> num_files_per_level;
        std::array<size_t, kTotalLevels> bytes_per_level;
        uint64_t total_compactions = 0;
        uint64_t total_bytes_written = 0; // includes compaction rewrites
        uint64_t total_bytes_read = 0;
        uint64_t user_bytes_written = 0;
        double write_amplification = 0.0; // total_bytes_written / user_bytes_written
        double read_amplification = 0.0;  // disk_reads / user_reads
    };

} // namespace bason_db
