#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace bason_db {

    // Core types
    enum class BasonType {
        Boolean,
        Array,
        String,
        Object,
        Number,
    };

    struct BasonRecord {
        BasonType type;
        std::string key;
        std::string value;                 // leaf types
        std::vector<BasonRecord> children; // container types (Array, Object)

        bool operator==(const BasonRecord& other) const = default;
    };

    // Encode a record to bytes. The encoder must choose short form when
    // both key and value fit in 15 bytes, long form otherwise.
    std::vector<uint8_t> bason_encode(const BasonRecord& record);

    // Decode a record from bytes. Returns the record and the number of
    // bytes consumed. Throws on malformed input.
    std::pair<BasonRecord, size_t> bason_decode(const uint8_t* data, size_t len);

    // Decode all records from a byte buffer (a stream of concatenated
    // records).
    std::vector<BasonRecord> bason_decode_all(const uint8_t* data, size_t len);

} // namespace bason_db
