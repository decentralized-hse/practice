#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace basonlite::codec {

enum class BasonType : uint8_t {
    Boolean = 1,
    Array   = 2,
    String  = 3,
    Object  = 4,
    Number  = 5
};

struct BasonRecord {
    BasonType type;
    std::string key;
    std::string value;
    std::vector<BasonRecord> children;
};

std::vector<uint8_t> bason_encode(const BasonRecord& record);

std::pair<BasonRecord, size_t> bason_decode(const uint8_t* data, size_t len);

} // namespace basonlite::codec