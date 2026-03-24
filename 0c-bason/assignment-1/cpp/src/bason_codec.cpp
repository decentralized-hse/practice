#include "bason_codec.h"

#include <stdexcept>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

namespace {

// BASON tag bytes
constexpr uint8_t TAG_BOOLEAN_SHORT = 'b';  // 0x62
constexpr uint8_t TAG_ARRAY_SHORT   = 'a';  // 0x61
constexpr uint8_t TAG_STRING_SHORT  = 's';  // 0x73
constexpr uint8_t TAG_OBJECT_SHORT  = 'o';  // 0x6F
constexpr uint8_t TAG_NUMBER_SHORT  = 'n';  // 0x6E

constexpr uint8_t TAG_BOOLEAN_LONG  = 'B';  // 0x42
constexpr uint8_t TAG_ARRAY_LONG    = 'A';  // 0x41
constexpr uint8_t TAG_STRING_LONG   = 'S';  // 0x53
constexpr uint8_t TAG_OBJECT_LONG   = 'O';  // 0x4F
constexpr uint8_t TAG_NUMBER_LONG   = 'N';  // 0x4E

constexpr size_t MAX_SHORT_LENGTH = 15;
constexpr size_t MAX_LONG_KEY_LENGTH = 255;

////////////////////////////////////////////////////////////////////////////////

uint8_t GetTagForType(EBasonType type, bool shortForm)
{
    if (shortForm) {
        switch (type) {
            case EBasonType::Boolean: return TAG_BOOLEAN_SHORT;
            case EBasonType::Array:   return TAG_ARRAY_SHORT;
            case EBasonType::String:  return TAG_STRING_SHORT;
            case EBasonType::Object:  return TAG_OBJECT_SHORT;
            case EBasonType::Number:  return TAG_NUMBER_SHORT;
        }
    } else {
        switch (type) {
            case EBasonType::Boolean: return TAG_BOOLEAN_LONG;
            case EBasonType::Array:   return TAG_ARRAY_LONG;
            case EBasonType::String:  return TAG_STRING_LONG;
            case EBasonType::Object:  return TAG_OBJECT_LONG;
            case EBasonType::Number:  return TAG_NUMBER_LONG;
        }
    }
    throw std::logic_error("Unknown BASON type");
}

EBasonType GetTypeFromTag(uint8_t tag)
{
    switch (tag) {
        case TAG_BOOLEAN_SHORT:
        case TAG_BOOLEAN_LONG:
            return EBasonType::Boolean;
        case TAG_ARRAY_SHORT:
        case TAG_ARRAY_LONG:
            return EBasonType::Array;
        case TAG_STRING_SHORT:
        case TAG_STRING_LONG:
            return EBasonType::String;
        case TAG_OBJECT_SHORT:
        case TAG_OBJECT_LONG:
            return EBasonType::Object;
        case TAG_NUMBER_SHORT:
        case TAG_NUMBER_LONG:
            return EBasonType::Number;
        default:
            throw std::runtime_error("Invalid BASON tag: 0x" + std::to_string(tag));
    }
}

bool IsShortForm(uint8_t tag)
{
    return tag >= 'a' && tag <= 'z';
}

bool IsContainerType(EBasonType type)
{
    return type == EBasonType::Array || type == EBasonType::Object;
}

void WriteLittleEndian32(std::vector<uint8_t>& buffer, uint32_t value)
{
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 24) & 0xFF);
}

uint32_t ReadLittleEndian32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

std::vector<uint8_t> EncodeChildren(const std::vector<TBasonRecord>& children)
{
    std::vector<uint8_t> result;
    for (const auto& child : children) {
        auto encoded = EncodeBason(child);
        result.insert(result.end(), encoded.begin(), encoded.end());
    }
    return result;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> EncodeBason(const TBasonRecord& record)
{
    std::vector<uint8_t> result;

    // For containers, encode children first to know the value length
    std::vector<uint8_t> valueData;
    
    if (IsContainerType(record.Type)) {
        valueData = EncodeChildren(record.Children);
    } else {
        valueData.assign(record.Value.begin(), record.Value.end());
    }

    size_t keyLen = record.Key.size();
    size_t valueLen = valueData.size();

    // Choose short or long form
    bool useShortForm = (keyLen <= MAX_SHORT_LENGTH && valueLen <= MAX_SHORT_LENGTH);

    // Write tag
    uint8_t tag = GetTagForType(record.Type, useShortForm);
    result.push_back(tag);

    if (useShortForm) {
        // Short form: lengths byte (upper nibble = key len, lower nibble = value len)
        uint8_t lengths = (keyLen << 4) | valueLen;
        result.push_back(lengths);
    } else {
        // Long form: val_len (4B LE) + key_len (1B)
        if (keyLen > MAX_LONG_KEY_LENGTH) {
            throw std::invalid_argument("Key length exceeds maximum (255 bytes)");
        }
        if (valueLen > UINT32_MAX) {
            throw std::invalid_argument("Value length exceeds maximum (4GB)");
        }
        
        WriteLittleEndian32(result, static_cast<uint32_t>(valueLen));
        result.push_back(static_cast<uint8_t>(keyLen));
    }

    // Write key
    result.insert(result.end(), record.Key.begin(), record.Key.end());

    // Write value
    result.insert(result.end(), valueData.begin(), valueData.end());

    return result;
}

std::pair<TBasonRecord, size_t> DecodeBason(const uint8_t* data, size_t len)
{
    if (len < 2) {
        throw std::runtime_error("Insufficient data for BASON record (need at least 2 bytes)");
    }

    TBasonRecord record;
    size_t offset = 0;

    // Read tag
    uint8_t tag = data[offset++];
    record.Type = GetTypeFromTag(tag);
    bool shortForm = IsShortForm(tag);

    size_t keyLen, valueLen;

    if (shortForm) {
        // Short form
        uint8_t lengths = data[offset++];
        keyLen = (lengths >> 4) & 0x0F;
        valueLen = lengths & 0x0F;
    } else {
        // Long form
        if (len < 6) {
            throw std::runtime_error("Insufficient data for long form BASON record");
        }
        valueLen = ReadLittleEndian32(&data[offset]);
        offset += 4;
        keyLen = data[offset++];
    }

    // Read key
    if (offset + keyLen > len) {
        throw std::runtime_error("Insufficient data for key");
    }
    record.Key.assign(reinterpret_cast<const char*>(&data[offset]), keyLen);
    offset += keyLen;

    // Read value
    if (offset + valueLen > len) {
        throw std::runtime_error("Insufficient data for value");
    }

    if (IsContainerType(record.Type)) {
        // Decode children
        size_t childOffset = 0;
        while (childOffset < valueLen) {
            auto [child, childSize] = DecodeBason(&data[offset + childOffset], valueLen - childOffset);
            record.Children.push_back(std::move(child));
            childOffset += childSize;
        }
    } else {
        // Leaf value
        record.Value.assign(reinterpret_cast<const char*>(&data[offset]), valueLen);
    }
    offset += valueLen;

    return {std::move(record), offset};
}

std::vector<TBasonRecord> DecodeBasonAll(const uint8_t* data, size_t len)
{
    std::vector<TBasonRecord> records;
    size_t offset = 0;

    while (offset < len) {
        auto [record, size] = DecodeBason(&data[offset], len - offset);
        records.push_back(std::move(record));
        offset += size;
    }

    return records;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
