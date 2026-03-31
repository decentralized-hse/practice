#pragma once

#include "mem_utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace basonlite {

constexpr std::size_t kDefaultPageSize = 4096;
constexpr std::size_t kBTreePageHeaderSize = 12;
constexpr std::size_t kDbHeaderSize = 32;

enum class PageType : std::uint8_t {
    Invalid  = 0x00,
    Interior = 0x01,
    Leaf     = 0x02,
    Overflow = 0x03,
    Freelist = 0x04,
};

struct Page {
    explicit Page(std::size_t page_size = kDefaultPageSize)
        : bytes(page_size, 0) {}

    std::size_t size() const noexcept {
        return bytes.size();
    }

    void zero() {
        std::memset(bytes.data(), 0, bytes.size());
    }

    std::vector<std::uint8_t> bytes;
};

struct DatabaseHeader {
    std::uint32_t version = 1;
    std::uint32_t page_size = static_cast<std::uint32_t>(kDefaultPageSize);
    std::uint32_t total_pages = 1;
    std::uint32_t free_head = 0;
    std::uint32_t root_page = 0;
    std::uint32_t reserved = 0;
};

inline void require_page_size(const Page& page) {
    if (page.size() < kBTreePageHeaderSize) {
        throw std::runtime_error("small page");
    }
}

inline std::uint16_t read_u16le(const std::vector<std::uint8_t>& buf, std::size_t off) {
    return utils::read_u16_ptr(&buf[off]);
}

inline std::uint32_t read_u32le(const std::vector<std::uint8_t>& buf, std::size_t off) {
    return utils::read_u32_ptr(&buf[off]);
}

inline void write_u16le(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t value) {
    utils::write_u16_ptr(&buf[off], value);
}

inline void write_u32le(std::vector<std::uint8_t>& buf, std::size_t off, std::uint32_t value) {
    utils::write_u32_ptr(&buf[off], value);
}

inline std::uint8_t page_type(const Page& page) {
    require_page_size(page);
    return page.bytes[0];
}

inline void set_page_type(Page& page, PageType type) {
    require_page_size(page);
    page.bytes[0] = static_cast<std::uint8_t>(type);
}

inline std::uint16_t cell_count(const Page& page) {
    require_page_size(page);
    return read_u16le(page.bytes, 1);
}

inline void set_cell_count(Page& page, std::uint16_t count) {
    require_page_size(page);
    write_u16le(page.bytes, 1, count);
}

inline std::uint16_t free_ofs(const Page& page) {
    require_page_size(page);
    return read_u16le(page.bytes, 3);
}

inline void set_free_ofs(Page& page, std::uint16_t ofs) {
    require_page_size(page);
    write_u16le(page.bytes, 3, ofs);
}

inline std::uint8_t frag(const Page& page) {
    require_page_size(page);
    return page.bytes[5];
}

inline void set_frag(Page& page, std::uint8_t value) {
    require_page_size(page);
    page.bytes[5] = value;
}

inline std::uint32_t right_child(const Page& page) {
    require_page_size(page);
    return read_u32le(page.bytes, 8);
}

inline void set_right_child(Page& page, std::uint32_t child_page_no) {
    require_page_size(page);
    write_u32le(page.bytes, 8, child_page_no);
}

inline std::size_t cell_pointer_offset(std::size_t index) {
    return kBTreePageHeaderSize + index * 2;
}

inline std::uint16_t get_cell_pointer(const Page& page, std::size_t index) {
    require_page_size(page);
    return read_u16le(page.bytes, cell_pointer_offset(index));
}

inline void set_cell_pointer(Page& page, std::size_t index, std::uint16_t ptr) {
    require_page_size(page);
    write_u16le(page.bytes, cell_pointer_offset(index), ptr);
}

constexpr std::array<std::uint8_t, 8> kDbMagic = {'B','A','S','O','N','L','T','1'};

inline DatabaseHeader read_db_header(const Page& page) {
    if (page.size() < kDbHeaderSize) {
        throw std::runtime_error("database header page too small");
    }

    if (std::memcmp(page.bytes.data(), kDbMagic.data(), kDbMagic.size()) != 0) {
        throw std::runtime_error("invalid database magic");
    }

    DatabaseHeader h;
    h.version = read_u32le(page.bytes, 8);
    h.page_size = read_u32le(page.bytes, 12);
    h.total_pages = read_u32le(page.bytes, 16);
    h.free_head = read_u32le(page.bytes, 20);
    h.root_page = read_u32le(page.bytes, 24);
    h.reserved = read_u32le(page.bytes, 28);
    return h;
}

inline void write_db_header(Page& page, const DatabaseHeader& h) {
    if (page.size() < kDbHeaderSize) {
        throw std::runtime_error("");
    }

    std::memcpy(page.bytes.data(), kDbMagic.data(), kDbMagic.size());
    write_u32le(page.bytes, 8, h.version);
    write_u32le(page.bytes, 12, h.page_size);
    write_u32le(page.bytes, 16, h.total_pages);
    write_u32le(page.bytes, 20, h.free_head);
    write_u32le(page.bytes, 24, h.root_page);
    write_u32le(page.bytes, 28, h.reserved);
}

} // namespace basonlite