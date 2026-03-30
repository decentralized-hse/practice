#pragma once

#include "codec/record.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace bason_db {

    struct VersionEdit {
        struct FileAdd {
            int level = 0;
            std::filesystem::path file;
            uint64_t file_size = 0;
            std::string first_key;
            std::string last_key;
            uint64_t min_offset = 0;
            uint64_t max_offset = 0;
        };

        struct FileRemove {
            int level = 0;
            std::filesystem::path file;
        };

        std::vector<FileAdd> additions;
        std::vector<FileRemove> removals;
        uint64_t flush_offset = 0;
    };

    BasonRecord to_bason(const VersionEdit::FileAdd& add);

    void from_bason(const BasonRecord& record, VersionEdit::FileAdd& out);

    BasonRecord to_bason(const VersionEdit::FileRemove& remove);

    void from_bason(const BasonRecord& record, VersionEdit::FileRemove& out);

    BasonRecord to_bason(const VersionEdit& edit);

    void from_bason(const BasonRecord& record, VersionEdit& out);

} // namespace bason_db
