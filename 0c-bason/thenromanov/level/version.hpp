#pragma once

#include "level/constants.hpp"

#include "sst/metadata.hpp"
#include "sst/reader.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace bason_db {

    struct FileInfo {
        SstMetadata meta;
        std::shared_ptr<SstReader> reader;
    };

    class Version {
    public:
        void add_file(int level, const SstMetadata& meta);

        void remove_file(int level, const std::filesystem::path& path);

        const std::vector<FileInfo>& files(int level) const;

        std::vector<FileInfo> sorted_files(int level) const;

        std::vector<FileInfo> overlapping_files(int level, const std::string& min_key,
                                                const std::string& max_key) const;

        size_t num_files(int level) const;

        size_t total_bytes(int level) const;

        std::vector<SstMetadata> all_metadata() const;

    private:
        std::array<std::vector<FileInfo>, kTotalLevels + 1> levels_;
    };

} // namespace bason_db
