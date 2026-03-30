#pragma once

#include "level/level_metrics.hpp"
#include "level/manifest.hpp"
#include "level/version.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace bason_db {

    struct CompactionOptions {
        size_t block_size = 4096;
        int bloom_bits_per_key = 10;
        size_t max_sst_size = 4 * 1024 * 1024; // 4 MB
    };

    struct CompactionResult {
        VersionEdit edit;
        size_t bytes_read = 0;
        size_t bytes_written = 0;
    };

    CompactionResult run_compaction(int source_level, const std::vector<FileInfo>& input_files,
                                    const std::vector<FileInfo>& target_files,
                                    const std::filesystem::path& sst_dir,
                                    std::atomic_uint64_t& next_file_number,
                                    uint64_t min_live_offset, const CompactionOptions& opts = {});

} // namespace bason_db
