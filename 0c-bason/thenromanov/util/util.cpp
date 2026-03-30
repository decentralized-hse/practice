#include "util/util.hpp"

namespace bason_db {

    namespace {

        size_t get_string_heap_size(const std::string& str) {
            if (str.capacity() <= 15) {
                return 0;
            }
            return str.capacity();
        }

        size_t get_dynamic_size(const BasonRecord& record) {
            size_t size = 0;

            size += get_string_heap_size(record.key);
            size += get_string_heap_size(record.value);

            size += record.children.capacity() * sizeof(BasonRecord);

            for (const auto& child : record.children) {
                size += get_dynamic_size(child);
            }

            return size;
        }

    } // namespace

    bool is_tombstone(const BasonRecord& record) {
        return record.type == BasonType::Boolean && record.value.empty();
    }

    BasonRecord make_tombstone(const std::string& key) {
        return BasonRecord{
            .type = BasonType::Boolean,
            .key = key,
            .value = "",
            .children = {},
        };
    }

    BasonRecord make_number_record(const std::string& key, uint64_t value) {
        return BasonRecord{
            .type = BasonType::Number,
            .key = key,
            .value = std::to_string(value),
            .children = {},
        };
    }

    BasonRecord make_string_record(const std::string& key, const std::string& value) {
        return BasonRecord{
            .type = BasonType::String,
            .key = key,
            .value = value,
            .children = {},
        };
    }

    BasonRecord make_blob_record(const std::string& key, const BasonRecord& payload) {
        auto bytes = bason_encode(payload);
        return BasonRecord{
            .type = BasonType::String,
            .key = key,
            .value = std::string{bytes.begin(), bytes.end()},
            .children = {},
        };
    }

    void extract_blob_record(const BasonRecord& child, BasonRecord& out) {
        const auto* data = reinterpret_cast<const uint8_t*>(child.value.data());
        auto result = bason_decode(data, child.value.size());
        out = std::move(result.first);
    }

    size_t record_size(const BasonRecord& record) {
        return sizeof(BasonRecord) + get_dynamic_size(record);
    }

    std::filesystem::path get_test_tmp_dir() {
        const char* test_tmpdir = std::getenv("TEST_TMPDIR");
        if (test_tmpdir == nullptr) {
            return std::filesystem::temp_directory_path();
        }
        return std::filesystem::path(test_tmpdir);
    }

} // namespace bason_db
