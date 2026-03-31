#pragma once

#include <cstdint>

namespace basonlite::utils {

inline std::uint16_t read_u16_ptr(const std::uint8_t *p) {
    return static_cast<std::uint16_t>(p[0])
           | (static_cast<std::uint16_t>(p[1]) << 8);
}

inline std::uint32_t read_u32_ptr(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0])
           | (static_cast<std::uint32_t>(p[1]) << 8)
           | (static_cast<std::uint32_t>(p[2]) << 16)
           | (static_cast<std::uint32_t>(p[3]) << 24);
}

inline void write_u16_ptr(std::uint8_t *p, std::uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v & 0xFF);
    p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
}

inline void write_u32_ptr(std::uint8_t *p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v & 0xFF);
    p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
}

} // namespace basonlite::utils