#include "sst/record.hpp"

#include "util/util.hpp"

namespace bason_db {

    namespace {

        std::string encode_internal_key(const std::string& user_key, uint64_t offset) {
            auto inverted = std::numeric_limits<uint64_t>::max() - offset;

            auto result = std::string{};
            result.reserve(user_key.size() + 1 + 8);

            result.append(user_key);

            result.push_back('\0');

            for (int i = 7; i >= 0; --i) {
                result.push_back(static_cast<char>((inverted >> (i * 8)) & 0xFF));
            }

            return result;
        }

    } // namespace

    BasonRecord to_bason(const DataRecord& data_record) {
        return BasonRecord{
            .type = BasonType::Object,
            .key = encode_internal_key(data_record.key, data_record.offset),
            .value = "",
            .children =
                {
                    make_string_record("key", data_record.key),
                    make_blob_record("record", data_record.record),
                    make_number_record("offset", data_record.offset),
                },
        };
    }

    void from_bason(const BasonRecord& record, DataRecord& out) {
        if (record.type != BasonType::Object) {
            throw std::runtime_error{"SST Data Record type must be \"BasonType::Object\""};
        }
        for (const auto& child : record.children) {
            if (child.key == "key" && child.type == BasonType::String) {
                out.key = child.value;
            } else if (child.key == "offset" && child.type == BasonType::Number) {
                out.offset = std::stoull(child.value);
            } else if (child.key == "record" && child.type == BasonType::String) {
                extract_blob_record(child, out.record);
            }
        }
    }

    BasonRecord to_bason(const IndexRecord& index_record) {
        return BasonRecord{
            .type = BasonType::Object,
            .key = index_record.first_key,
            .value = "",
            .children =
                {
                    make_number_record("block_offset", index_record.block_offset),
                },
        };
    }

    void from_bason(const BasonRecord& record, IndexRecord& out) {
        if (record.type != BasonType::Object) {
            throw std::runtime_error{"SST Index Record type must be \"BasonType::Object\""};
        }

        out.first_key = record.key;

        for (const auto& child : record.children) {
            if (child.key == "block_offset" && child.type == BasonType::Number) {
                out.block_offset = std::stoull(child.value);
            }
        }
    }

} // namespace bason_db
