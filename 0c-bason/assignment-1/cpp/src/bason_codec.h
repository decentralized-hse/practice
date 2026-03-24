#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

enum class EBasonType {
    Boolean,
    Array,
    String,
    Object,
    Number
};

struct TBasonRecord {
    EBasonType Type;
    std::string Key;
    std::string Value;  // For leaf types
    std::vector<TBasonRecord> Children;  // For container types (Array, Object)

    TBasonRecord() = default;
    
    TBasonRecord(EBasonType type)
        : Type(type)
    { }

    TBasonRecord(EBasonType type, std::string key, std::string value)
        : Type(type)
        , Key(std::move(key))
        , Value(std::move(value))
    { }
};

////////////////////////////////////////////////////////////////////////////////

// Encode a record to bytes
// The encoder MUST choose short form when both key and value fit in 15 bytes
std::vector<uint8_t> EncodeBason(const TBasonRecord& record);

// Decode a record from bytes
// Returns the record and the number of bytes consumed
// Throws on malformed input
std::pair<TBasonRecord, size_t> DecodeBason(const uint8_t* data, size_t len);

// Decode all records from a byte buffer (a stream of concatenated records)
std::vector<TBasonRecord> DecodeBasonAll(const uint8_t* data, size_t len);

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
