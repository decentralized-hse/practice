#include "wal/record.hpp"

#include "util/util.hpp"

#include <stdexcept>

namespace bason_db {

    BasonRecord to_bason(const WalRecord& wal) {
        auto encoded_bytes = bason_encode(wal.record);
        auto encoded_record = std::string{encoded_bytes.begin(), encoded_bytes.end()};

        return BasonRecord{
            .type = BasonType::Object,
            .key = "",
            .value = "",
            .children =
                {
                    make_string_record("key", wal.key),
                    make_blob_record("record", wal.record),
                },
        };
    }

    void from_bason(const BasonRecord& record, WalRecord& out) {
        if (record.type != BasonType::Object) {
            throw std::runtime_error{"WAL Record type must be \"BasonType::Object\""};
        }
        for (const auto& child : record.children) {
            if (child.key == "key" && child.type == BasonType::String) {
                out.key = child.value;
            } else if (child.key == "record" && child.type == BasonType::String) {
                extract_blob_record(child, out.record);
            }
        }
    }

} // namespace bason_db
