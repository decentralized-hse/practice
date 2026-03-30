#include "level/version.hpp"

#include <algorithm>

namespace bason_db {

    void Version::add_file(int level, const SstMetadata& meta) {
        if (level < 0 || level >= kTotalLevels) {
            return;
        }

        auto info = FileInfo{
            .meta = meta,
            .reader = std::make_shared<SstReader>(SstReader::open(meta.path)),
        };
        levels_[static_cast<size_t>(level)].emplace_back(std::move(info));
    }

    void Version::remove_file(int level, const std::filesystem::path& path) {
        if (level < 0 || level >= kTotalLevels) {
            return;
        }

        auto& files = levels_[static_cast<size_t>(level)];
        std::erase_if(files, [&path](const FileInfo& info) { return info.meta.path == path; });
    }

    const std::vector<FileInfo>& Version::files(int level) const {
        if (level < 0 || level >= kTotalLevels) {
            return levels_.back();
        }
        return levels_[static_cast<size_t>(level)];
    }

    std::vector<FileInfo> Version::sorted_files(int level) const {
        if (level < 0 || level >= kTotalLevels) {
            return {};
        }

        auto result = levels_[static_cast<size_t>(level)];
        std::sort(result.begin(), result.end(), [](const FileInfo& lhs, const FileInfo& rhs) {
            return lhs.meta.first_key < rhs.meta.first_key;
        });
        return result;
    }

    std::vector<FileInfo> Version::overlapping_files(int level, const std::string& min_key,
                                                     const std::string& max_key) const {
        if (level < 0 || level >= kTotalLevels) {
            return {};
        }

        auto result = std::vector<FileInfo>{};
        for (const auto& info : levels_[static_cast<size_t>(level)]) {
            bool overlaps = true;
            if (!max_key.empty() && info.meta.first_key >= max_key) {
                overlaps = false;
            }
            if (!min_key.empty() && info.meta.last_key < min_key) {
                overlaps = false;
            }
            if (overlaps) {
                result.emplace_back(info);
            }
        }
        return result;
    }

    size_t Version::num_files(int level) const {
        if (level < 0 || level >= kTotalLevels) {
            return 0;
        }
        return levels_[static_cast<size_t>(level)].size();
    }

    size_t Version::total_bytes(int level) const {
        if (level < 0 || level >= kTotalLevels) {
            return 0;
        }

        size_t total = 0;
        for (const auto& info : levels_[static_cast<size_t>(level)]) {
            total += info.meta.file_size;
        }
        return total;
    }

    std::vector<SstMetadata> Version::all_metadata() const {
        auto result = std::vector<SstMetadata>{};
        for (const auto& level : levels_) {
            for (const auto& info : level) {
                result.emplace_back(info.meta);
            }
        }
        return result;
    }

} // namespace bason_db
