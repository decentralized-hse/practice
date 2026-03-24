#pragma once

#include <cstdint>
#include <string>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

// RON64 encoding for array indices with lexicographic sortability.
// Alphabet: 0-9A-Z_a-z~ (64 characters in ASCII order)
// Numbers are encoded in big-endian digit order (most significant first).

// Encode a non-negative integer to RON64 string
std::string EncodeRon64(uint64_t value);

// Decode a RON64 string to integer
// Throws std::invalid_argument on invalid characters
uint64_t DecodeRon64(const std::string& str);

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
