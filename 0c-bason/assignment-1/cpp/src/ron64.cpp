#include "ron64.h"

#include <stdexcept>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

namespace {

// RON64 alphabet in ASCII order: 0-9A-Z_a-z~
constexpr char RON64_ALPHABET[] = 
    "0123456789"                  // 0-9
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  // 10-35
    "_"                           // 36
    "abcdefghijklmnopqrstuvwxyz"  // 37-62
    "~";                          // 63

constexpr size_t RON64_BASE = 64;

// Build reverse lookup table for decoding
constexpr int BuildDecodeTable(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'Z') return 10 + (ch - 'A');
    if (ch == '_') return 36;
    if (ch >= 'a' && ch <= 'z') return 37 + (ch - 'a');
    if (ch == '~') return 63;
    return -1;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

std::string EncodeRon64(uint64_t value)
{
    // Special case: zero
    if (value == 0) {
        return "0";
    }

    // Convert to base-64 (big-endian: most significant digit first)
    std::string result;
    while (value > 0) {
        result += RON64_ALPHABET[value % RON64_BASE];
        value /= RON64_BASE;
    }

    // Reverse to get big-endian order
    std::reverse(result.begin(), result.end());
    
    return result;
}

uint64_t DecodeRon64(const std::string& str)
{
    if (str.empty()) {
        throw std::invalid_argument("Empty RON64 string");
    }

    uint64_t result = 0;
    
    for (char ch : str) {
        int digit = BuildDecodeTable(ch);
        if (digit < 0) {
            throw std::invalid_argument("Invalid RON64 character: " + std::string(1, ch));
        }

        // Check for overflow
        if (result > (UINT64_MAX / RON64_BASE)) {
            throw std::overflow_error("RON64 value overflow");
        }
        
        result = result * RON64_BASE + digit;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
