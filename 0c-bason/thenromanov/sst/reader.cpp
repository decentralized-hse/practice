#include "sst/reader.hpp"

#include "sst/constants.hpp"
#include "sst/util.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>

namespace bason_db {

    namespace {

        uint64_t read_u64_le(const uint8_t* p) {
            uint64_t v = 0;
            std::memcpy(&v, p, 8);
            return v;
        }

    } // namespace

    SstReader SstReader::open(const std::filesystem::path& path) {
        auto reader = SstReader{};

        reader.path_ = path;
        reader.meta_.path = path;

        auto fs = std::ifstream{path, std::ios::binary | std::ios::ate};
        if (!fs) {
            throw std::runtime_error{"Cannot open file: " + path.string()};
        }

        size_t fs_size = fs.tellg();
        if (fs_size < kFooterSize) {
            throw std::runtime_error{"File too small: " + path.string()};
        }

        fs.seekg(static_cast<std::streamoff>(fs_size - kFooterSize));
        auto footer = std::array<uint8_t, kFooterSize>{};
        fs.read(reinterpret_cast<char*>(footer.data()), footer.size());

        if (std::memcmp(footer.data() + kFooterSize - kSstTag.size(), kSstTag.data(),
                        kSstTag.size()) != 0) {
            throw std::runtime_error{"Invalid magic in footer: " + path.string()};
        }

        uint64_t filter_offset = read_u64_le(footer.data() + 0);
        // uint64_t filter_size = read_u64_le(footer.data() + 8);
        uint64_t index_offset = read_u64_le(footer.data() + 16);
        uint64_t index_size = read_u64_le(footer.data() + 24);
        uint64_t record_count = read_u64_le(footer.data() + 32);

        reader.meta_.record_count = record_count;
        reader.meta_.file_size = fs_size;

        fs.seekg(0);
        auto data = std::vector<uint8_t>(fs_size);
        fs.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(fs_size));

        auto index_records = std::vector<IndexRecord>{};
        if (index_size > 0 && index_offset + index_size <= fs_size - kFooterSize) {
            auto records = bason_decode_all(data.data() + index_offset, index_size);
            for (const auto& record : records) {
                from_bason(record, index_records.emplace_back());
            }
        }

        uint64_t data_size = filter_offset;
        reader.records_.reserve(record_count);

        for (size_t i = 0; i < index_records.size(); ++i) {
            uint64_t block_start = index_records[i].block_offset;
            uint64_t block_end =
                (i + 1 < index_records.size()) ? index_records[i + 1].block_offset : data_size;

            if (block_start >= data_size || block_end > data_size || block_start >= block_end) {
                continue;
            }

            auto block_len = static_cast<size_t>(block_end - block_start);
            auto block_records = bason_decode_all(data.data() + block_start, block_len);

            for (const auto& record : block_records) {
                from_bason(record, reader.records_.emplace_back());
            }
        }

        reader.meta_.first_key = reader.records_.empty() ? "" : reader.records_.front().key;
        reader.meta_.last_key = reader.records_.empty() ? "" : reader.records_.back().key;

        if (!reader.records_.empty()) {
            auto min_off = std::numeric_limits<uint64_t>::max();
            uint64_t max_off = 0;
            for (const auto& entry : reader.records_) {
                if (entry.offset < min_off) {
                    min_off = entry.offset;
                }
                if (entry.offset > max_off) {
                    max_off = entry.offset;
                }
            }
            reader.meta_.min_offset = min_off;
            reader.meta_.max_offset = max_off;
        }

        return reader;
    }

    std::optional<std::pair<BasonRecord, uint64_t>> SstReader::get(const std::string& key,
                                                                   uint64_t max_offset) {
        auto it = std::lower_bound(
            records_.begin(), records_.end(), key,
            [](const DataRecord& entry, const std::string& key) { return entry.key < key; });

        while (it != records_.end() && it->key == key) {
            if (it->offset <= max_offset) {
                return std::make_pair(it->record, it->offset);
            }
            ++it;
        }
        return std::nullopt;
    }

    SstIterator SstReader::scan(const std::string& start, const std::string& end) {
        return SstIterator{records_, start, end};
    }

    bool SstReader::may_contain(const std::string& key) {
        auto it = std::lower_bound(
            records_.begin(), records_.end(), key,
            [](const DataRecord& entry, const std::string& key) { return entry.key < key; });
        return (it != records_.end() && it->key == key);
    }

    SstMetadata SstReader::metadata() const {
        return meta_;
    }

} // namespace bason_db
