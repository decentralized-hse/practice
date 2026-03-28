#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace bason_db {

    struct SstMetadata {
        std::filesystem::path path;
        uint64_t file_size = 0;
        std::string first_key;
        std::string last_key;
        uint64_t record_count = 0;
        int level = 0;
        uint64_t min_offset = 0;
        uint64_t max_offset = 0;
    };

} // namespace bason_db
