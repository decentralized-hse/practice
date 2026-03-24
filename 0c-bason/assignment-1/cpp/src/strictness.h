#pragma once

#include "bason_codec.h"

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

// Strictness bitmask bits
namespace NStrictness {
    constexpr uint16_t SHORTEST_ENCODING     = 0x001;  // Bit 0
    constexpr uint16_t CANONICAL_NUMBER      = 0x002;  // Bit 1
    constexpr uint16_t VALID_UTF8            = 0x004;  // Bit 2
    constexpr uint16_t NO_DUPLICATE_KEYS     = 0x008;  // Bit 3
    constexpr uint16_t CONTIGUOUS_ARRAY      = 0x010;  // Bit 4
    constexpr uint16_t ORDERED_ARRAY         = 0x020;  // Bit 5
    constexpr uint16_t SORTED_OBJECT_KEYS    = 0x040;  // Bit 6
    constexpr uint16_t CANONICAL_BOOLEAN     = 0x080;  // Bit 7
    constexpr uint16_t MINIMAL_RON64         = 0x100;  // Bit 8
    constexpr uint16_t CANONICAL_PATH        = 0x200;  // Bit 9
    constexpr uint16_t NO_MIXING             = 0x400;  // Bit 10

    // Predefined levels
    constexpr uint16_t PERMISSIVE = 0x000;
    constexpr uint16_t STANDARD   = 0x1FF;  // Bits 0-8
    constexpr uint16_t STRICT     = 0x7FF;  // Bits 0-10
}

// Validate a record against a strictness bitmask
// Returns true if the record conforms to all enabled rules
// For container types, validation is recursive
bool ValidateBason(const TBasonRecord& record, uint16_t strictness);

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
