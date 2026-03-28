#include "sst/writer.hpp"

#include "sst/constants.hpp"
#include "sst/util.hpp"

#include <array>
#include <format>
#include <fstream>
#include <stdexcept>

namespace bason_db {

    namespace {

        void write_u64_le(std::ofstream& fs, uint64_t v) {
            auto buf = std::array<uint8_t, 8>{};
            std::memcpy(buf.data(), &v, 8);
            fs.write(reinterpret_cast<const char*>(buf.data()), 8);
        }

    } // namespace

    SstWriter SstWriter::open(const std::filesystem::path& path) {
        auto writer = SstWriter{};
        writer.path_ = path;
        return writer;
    }

    SstWriter SstWriter::open(const std::filesystem::path& path, Options opts) {
        auto writer = SstWriter{};
        writer.path_ = path;
        writer.opts_ = std::move(opts);
        return writer;
    }

    void SstWriter::add(const std::string& key, const BasonRecord& record, uint64_t offset) {
        auto entry = to_bason(DataRecord{
            .key = key,
            .record = record,
            .offset = offset,
        });

        if (!last_internal_key_.empty() && entry.key <= last_internal_key_) {
            throw std::runtime_error{
                std::format("Keys must be in strictly ascending order, got '{}' after '{}'", key,
                            last_user_key_)};
        }
        last_internal_key_ = entry.key;
        last_user_key_ = key;

        if (first_user_key_.empty()) {
            first_user_key_ = key;
        }

        if (offset < min_offset_) {
            min_offset_ = offset;
        }
        if (offset > max_offset_) {
            max_offset_ = offset;
        }
        records_.emplace_back(std::move(entry));
    }

    SstMetadata SstWriter::finish() {
        if (path_.has_parent_path()) {
            std::filesystem::create_directories(path_.parent_path());
        }

        auto fs = std::ofstream{path_, std::ios::binary | std::ios::trunc};
        if (!fs) {
            throw std::runtime_error{"Cannot open file: " + path_.string()};
        }

        auto data_buf = std::vector<uint8_t>{};

        auto index_records = std::vector<IndexRecord>{};

        size_t block_start = 0;
        size_t block_bytes = 0;
        auto block_first_key = std::string{};

        for (size_t i = 0; i < records_.size(); ++i) {
            const auto& entry = records_[i];

            if (block_bytes == 0) {
                block_first_key = entry.key;
                block_start = data_buf.size();
            }

            auto bytes = bason_encode(entry);

            data_buf.append_range(bytes);
            block_bytes += bytes.size();

            if (block_bytes >= opts_.block_size) {
                index_records.emplace_back(IndexRecord{
                    .first_key = block_first_key,
                    .block_offset = static_cast<uint64_t>(block_start),
                });
                block_bytes = 0;
            }
        }

        if (block_bytes > 0) {
            index_records.emplace_back(IndexRecord{
                .first_key = block_first_key,
                .block_offset = static_cast<uint64_t>(block_start),
            });
        }

        fs.write(reinterpret_cast<const char*>(data_buf.data()),
                 static_cast<std::streamsize>(data_buf.size()));
        const uint64_t data_size = data_buf.size();

        const uint64_t filter_offset = data_size;
        const uint64_t filter_size = 0;

        const uint64_t index_offset = filter_offset + filter_size;
        auto index_buf = std::vector<uint8_t>{};
        for (const auto& index_entry : index_records) {
            index_buf.append_range(bason_encode(to_bason(index_entry)));
        }
        fs.write(reinterpret_cast<const char*>(index_buf.data()),
                 static_cast<std::streamsize>(index_buf.size()));
        const uint64_t index_size = index_buf.size();

        write_u64_le(fs, filter_offset);
        write_u64_le(fs, filter_size);
        write_u64_le(fs, index_offset);
        write_u64_le(fs, index_size);
        write_u64_le(fs, static_cast<uint64_t>(records_.size()));

        fs.write(kSstTag.data(), kSstTag.size());

        fs.flush();
        fs.close();

        return SstMetadata{
            .path = path_,
            .file_size = std::filesystem::file_size(path_),
            .first_key = first_user_key_,
            .last_key = last_user_key_,
            .record_count = records_.size(),
            .level = 0,
            .min_offset = records_.empty() ? 0 : min_offset_,
            .max_offset = records_.empty() ? 0 : max_offset_,
        };
    }

} // namespace bason_db
