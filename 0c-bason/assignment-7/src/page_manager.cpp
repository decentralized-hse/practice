#include "page_manager.hpp"
#include "mem_utils.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace basonlite {
namespace {

constexpr std::size_t kPageLinkOfs = 1;

std::uint32_t page_next_ptr(const Page& page) {
    return utils::read_u32_ptr(page.bytes.data() + kPageLinkOfs);
}

void set_page_next_ptr(Page& page, std::uint32_t next) {
    utils::write_u32_ptr(page.bytes.data() + kPageLinkOfs, next);
}

} // namespace

static std::runtime_error io_error(const std::string& msg) {
    return std::runtime_error("PageManager: " + msg);
}

PageManager::PageManager(std::string path, std::size_t page_size)
        : db_path_(std::move(path))
        , wal_dir_(db_path_ + ".wal")
        , page_size_(page_size)
        , wal_writer_(wal::WalWriter::open(wal_dir_))
        , wal_reader_(wal::WalReader::open(wal_dir_)) {
    if (page_size_ < kDbHeaderSize) {
        throw std::invalid_argument("small page_size");
    }
    ensure_initialized();
    load_wal_cache_from_disk();
}

PageManager PageManager::open(const std::string& path, std::size_t page_size) {
    return PageManager(path, page_size);
}

std::uint64_t PageManager::file_size() const {
    std::ifstream in(db_path_, std::ios::binary | std::ios::ate);
    if (!in) {
        return 0;
    }
    const auto pos = in.tellg();
    if (pos < 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(pos);
}

void PageManager::ensure_initialized() {
    std::ifstream in(db_path_, std::ios::binary);
    if (!in.good()) {
        std::ofstream out(db_path_, std::ios::binary);
        if (!out) {
            throw io_error("cannot create database file");
        }
    }

    const auto sz = file_size();
    if (sz == 0) {
        Page header_page(page_size_);
        header_page.zero();

        DatabaseHeader h;
        h.version = 1;
        h.page_size = static_cast<std::uint32_t>(page_size_);
        h.total_pages = 1;
        h.free_head = 0;
        h.root_page = 0;
        h.reserved = 0;

        write_db_header(header_page, h);
        write_raw_page(1, header_page);
        wal_cache_[1] = header_page;
        return;
    }

    if (sz % page_size_ != 0) {
        throw io_error("database file size is not a multiple of page size");
    }

    Page header_page = read_raw_page(1);
    const DatabaseHeader h = read_db_header(header_page);
    if (h.page_size != page_size_) {
        throw io_error("page size mismatch with existing database file");
    }
}

Page PageManager::read_raw_page(std::uint32_t page_no) const {
    if (page_no == 0) {
        throw io_error("page numbers are 1-indexed");
    }

    std::ifstream in(db_path_, std::ios::binary);
    if (!in) {
        throw io_error("cannot open database file for reading");
    }

    const std::uint64_t offset = static_cast<std::uint64_t>(page_no - 1) * page_size_;
    const auto sz = file_size();
    if (offset + page_size_ > sz) {
        throw io_error("attempt to read beyond end of file");
    }

    in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

    Page page(page_size_);
    if (!in.read(reinterpret_cast<char*>(page.bytes.data()), static_cast<std::streamsize>(page_size_))) {
        throw io_error("failed to read page");
    }

    return page;
}

void PageManager::write_raw_page(std::uint32_t page_no, const Page& page) const {
    if (page_no == 0) {
        throw io_error("page numbers are 1-indexed");
    }
    if (page.size() != page_size_) {
        throw io_error("page size mismatch");
    }

    std::fstream io(db_path_, std::ios::binary | std::ios::in | std::ios::out);
    if (!io) {
        throw io_error("cannot open database file for writing");
    }

    const std::uint64_t offset = static_cast<std::uint64_t>(page_no - 1) * page_size_;
    io.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!io.write(reinterpret_cast<const char*>(page.bytes.data()), static_cast<std::streamsize>(page_size_))) {
        throw io_error("failed to write page");
    }
    io.flush();
}

DatabaseHeader PageManager::load_header() const {
    Page header_page = read_page(1);
    return read_db_header(header_page);
}

void PageManager::save_header(const DatabaseHeader& h) {
    Page header_page = read_page(1);
    write_db_header(header_page, h);

    wal_cache_[1] = header_page;
    wal_writer_.append(record_from_page(1, header_page));
    write_raw_page(1, header_page);
}

DatabaseHeader PageManager::header() const {
    return load_header();
}

void PageManager::set_root_page(std::uint32_t page_no) {
    DatabaseHeader h = load_header();
    h.root_page = page_no;
    save_header(h);
}

std::uint32_t PageManager::root_page() const {
    return load_header().root_page;
}

codec::BasonRecord PageManager::record_from_page(std::uint32_t page_no, const Page& page) {
    codec::BasonRecord rec;
    rec.type = codec::BasonType::Object;
    rec.key = std::to_string(page_no);
    rec.value.assign(reinterpret_cast<const char*>(page.bytes.data()), page.bytes.size());
    rec.children.clear();
    return rec;
}

void PageManager::load_wal_cache_from_disk() {
    wal_cache_.clear();
    wal_reader_.recover();
}

Page PageManager::read_page(std::uint32_t page_no) const {
    const auto it = wal_cache_.find(page_no);
    if (it != wal_cache_.end()) {
        return it->second;
    }
    return read_raw_page(page_no);
}

void PageManager::write_page(std::uint32_t page_no, const Page& page) {
    if (page.size() != page_size_) {
        throw io_error("page size mismatch");
    }
    if (page_no == 0) {
        throw io_error("page numbers are 1-indexed");
    }

    wal_cache_[page_no] = page;
    wal_writer_.append(record_from_page(page_no, page));
    write_raw_page(page_no, page);

    DatabaseHeader h = load_header();
    if (page_no > h.total_pages) {
        h.total_pages = page_no;
        save_header(h);
    }
}

std::uint32_t PageManager::allocate_page() {
    DatabaseHeader h = load_header();

    if (h.free_head != 0) {
        const std::uint32_t page_no = h.free_head;
        const Page free_page = read_page(page_no);
        const std::uint32_t next_free = page_next_ptr(free_page);

        h.free_head = next_free;
        save_header(h);

        Page blank(page_size_);
        blank.zero();
        write_page(page_no, blank);
        return page_no;
    }

    const std::uint32_t page_no = h.total_pages + 1;
    h.total_pages = page_no;
    save_header(h);

    Page blank(page_size_);
    blank.zero();
    write_page(page_no, blank);
    return page_no;
}

void PageManager::free_page(std::uint32_t page_no) {
    if (page_no == 0) {
        throw io_error("page numbers are 1-indexed");
    }
    if (page_no == 1) {
        throw io_error("page 1 is reserved for database header");
    }

    DatabaseHeader h = load_header();

    Page freed(page_size_);
    freed.zero();
    set_page_type(freed, PageType::Freelist);
    set_page_next_ptr(freed, h.free_head);

    write_page(page_no, freed);

    h.free_head = page_no;
    save_header(h);
}

void PageManager::checkpoint() {
    for (const auto& [page_no, page] : wal_cache_) {
        write_raw_page(page_no, page);
    }

    wal_writer_.checkpoint();
    wal_writer_.sync();
    wal::wal_truncate_before(wal_dir_, std::numeric_limits<std::uint64_t>::max());
    wal_cache_.clear();
}

} // namespace basonlite