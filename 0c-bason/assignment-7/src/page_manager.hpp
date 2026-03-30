#pragma once

#include "page.hpp"
#include "wal/wal.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>

namespace basonlite {

class PageManager {
public:
    static PageManager open(const std::string& path, std::size_t page_size = kDefaultPageSize);

    Page read_page(std::uint32_t page_no) const;
    void write_page(std::uint32_t page_no, const Page& page);

    std::uint32_t allocate_page();
    void free_page(std::uint32_t page_no);

    DatabaseHeader header() const;
    void set_root_page(std::uint32_t page_no);
    std::uint32_t root_page() const;

    std::size_t page_size() const noexcept { return page_size_; }

    void checkpoint();

private:
    PageManager(std::string path, std::size_t page_size);

    void ensure_initialized();
    std::uint64_t file_size() const;

    Page read_raw_page(std::uint32_t page_no) const;
    void write_raw_page(std::uint32_t page_no, const Page& page) const;

    DatabaseHeader load_header() const;
    void save_header(const DatabaseHeader& h);

    void load_wal_cache_from_disk();
    static codec::BasonRecord record_from_page(std::uint32_t page_no, const Page& page);
private:
    std::string db_path_;
    std::string wal_dir_;
    std::size_t page_size_;

    wal::WalWriter wal_writer_;
    wal::WalReader wal_reader_;

    mutable std::unordered_map<std::uint32_t, Page> wal_cache_;
};

} // namespace basonlite