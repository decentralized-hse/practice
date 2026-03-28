#include "wal/reader.hpp"

#include "wal/constants.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace bason_db {

    namespace {

        struct SegmentInfo {
            std::filesystem::path path;
            uint64_t start_offset = 0;
        };

        std::vector<SegmentInfo> list_segments(const std::filesystem::path& dir) {
            auto segments = std::vector<SegmentInfo>{};

            if (!std::filesystem::exists(dir)) {
                return segments;
            }

            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.path().extension() == kWalExtension) {
                    try {
                        uint64_t offset = std::stoull(entry.path().stem().string());
                        segments.emplace_back(SegmentInfo{
                            .path = entry.path(),
                            .start_offset = offset,
                        });
                    } catch (...) {
                    }
                }
            }

            std::sort(segments.begin(), segments.end(),
                      [](const SegmentInfo& lhs, const SegmentInfo& rhs) {
                          return lhs.start_offset < rhs.start_offset;
                      });

            return segments;
        }

        struct SegmentState {
            std::vector<WalIterator::Entry> committed_entries;
            uint64_t safe_truncate_offset = 0;
            bool has_checkpoint = false;
        };

        SegmentState read_segment(const std::filesystem::path& path, uint64_t seg_start) {
            auto result = SegmentState{};
            result.safe_truncate_offset = seg_start + kHeaderSize;
            result.has_checkpoint = false;

            auto fs = std::ifstream{path, std::ios::binary | std::ios::ate};
            if (!fs) {
                return result;
            }

            size_t fs_size = fs.tellg();
            if (fs_size < kHeaderSize) {
                return result;
            }

            fs.seekg(0);
            auto data = std::vector<uint8_t>(fs_size);
            fs.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(fs_size));

            // Validate header magic
            if (std::memcmp(data.data(), kWalTag.data(), kWalTag.size()) != 0) {
                return result;
            }

            size_t pos = kHeaderSize;
            auto pending_entries = std::vector<WalIterator::Entry>{};
            uint64_t checkpoint_end = seg_start + kHeaderSize;

            while (pos < fs_size) {
                uint8_t tag = data[pos];

                if (tag == kCheckpointTag) {
                    // Checkpoint: 'H' + 4-byte CRC
                    if (pos + 5 > fs_size) {
                        break;
                    }

                    for (auto& e : pending_entries) {
                        result.committed_entries.emplace_back(std::move(e));
                    }

                    pending_entries.clear();
                    checkpoint_end = seg_start + pos + 5;
                    result.safe_truncate_offset = checkpoint_end;
                    result.has_checkpoint = true;
                    pos += 5;

                } else {
                    size_t remaining = fs_size - pos;
                    try {
                        auto&& [record, consumed] = bason_decode(data.data() + pos, remaining);

                        size_t padded = (consumed + 7) & ~7ULL;
                        if (pos + padded > fs_size) {
                            break;
                        }

                        uint64_t global_off = seg_start + pos;
                        pending_entries.emplace_back(WalIterator::Entry{
                            .offset = global_off,
                            .record = std::move(record),
                        });
                        pos += padded;
                    } catch (...) {
                        break;
                    }
                }
            }

            return result;
        }

    } // namespace

    WalReader WalReader::open(const std::filesystem::path& dir) {
        auto reader = WalReader{};
        reader.dir_ = dir;
        return reader;
    }

    uint64_t WalReader::recover() {
        auto segments = list_segments(dir_);
        if (segments.empty()) {
            return 0;
        }

        uint64_t last_segment_safe_truncate_offset = 0;
        auto last_segment_path = std::filesystem::path{};
        uint64_t last_segment_safe_end = 0;

        for (const auto& segment : segments) {
            auto result = read_segment(segment.path, segment.start_offset);
            if (result.has_checkpoint) {
                last_segment_safe_truncate_offset = result.safe_truncate_offset;
                last_segment_path = segment.path;
                last_segment_safe_end = result.safe_truncate_offset - segment.start_offset;
            }
        }

        if (!last_segment_path.empty()) {
            std::filesystem::resize_file(last_segment_path, last_segment_safe_end);
        }

        for (const auto& segment : segments) {
            if (segment.start_offset > last_segment_safe_truncate_offset) {
                std::filesystem::remove(segment.path);
            }
        }

        return last_segment_safe_truncate_offset;
    }

    WalIterator WalReader::scan(uint64_t from_offset) {
        auto segments = list_segments(dir_);
        auto all_entries = std::vector<WalIterator::Entry>{};

        for (const auto& segment : segments) {
            auto fs_size = std::filesystem::file_size(segment.path);
            if (segment.start_offset + fs_size <= from_offset) {
                continue;
            }

            auto result = read_segment(segment.path, segment.start_offset);
            for (auto& entry : result.committed_entries) {
                if (entry.offset >= from_offset) {
                    all_entries.emplace_back(std::move(entry));
                }
            }
        }

        return WalIterator(std::move(all_entries));
    }

} // namespace bason_db
