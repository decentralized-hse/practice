#pragma once

#include "bason_record.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class WalIterator;

class WalWriter {
public:
    static WalWriter open(const std::string& dir);
    uint64_t append_buffered(const BasonRecord& record);
    std::uint64_t append(const BasonRecord& record);
    void checkpoint();
    void sync();
    void sync_periodic(bool force = false);
    void rotate(std::uint64_t max_segment_size);

    ~WalWriter();

    WalWriter(const WalWriter&) = delete;
    WalWriter& operator=(const WalWriter&) = delete;
    WalWriter(WalWriter&&) noexcept;
    WalWriter& operator=(WalWriter&&) noexcept;

private:
    WalWriter() = default;

    void close_current_segment() noexcept;
    void open_segment(std::uint64_t start_offset, bool create_new);
    void flush_pending_to_disk();
    void update_hasher_with_file_bytes_after_header();

    /// Максимум пользовательской буферизации перед write() (для append_buffered).
    /// append() сбрасывает запись сразу; держать сотни КБ+ в RAM для WAL обычно не нужно.
    static constexpr std::size_t kFlushThreshold = 4096;
    std::filesystem::path dir_;
    std::filesystem::path current_path_;
    std::uint64_t current_segment_start_offset_ = 0;
    std::uint64_t current_segment_logical_size_ = 0;
    std::uint64_t committed_stream_offset_ = 0;
    std::vector<std::uint8_t> pending_;
    int fd_ = -1;
    std::uint64_t last_sync_offset_ = 0;
    
    struct Blake3State;
    Blake3State* hasher_ = nullptr;
};

class WalReader {
public:
    static WalReader open(const std::string& dir);

    std::uint64_t recover();
    WalIterator scan(std::uint64_t from_offset);

    WalReader(const WalReader&) = delete;
    WalReader& operator=(const WalReader&) = delete;
    WalReader(WalReader&&) noexcept;
    WalReader& operator=(WalReader&&) noexcept;
    ~WalReader();

private:
    WalReader() = default;

    std::filesystem::path dir_;
    bool recovered_ = false;
    uint64_t safe_offset_ = 0;

};

class WalIterator {
public:
    bool valid() const noexcept;
    void next();
    std::uint64_t offset() const;
    const BasonRecord& record() const;

    WalIterator(WalIterator&&) noexcept;
    WalIterator& operator=(WalIterator&&) noexcept;
    WalIterator(const WalIterator&) = delete;
    WalIterator& operator=(const WalIterator&) = delete;
    ~WalIterator();

private:
    friend class WalReader;
    WalIterator() = default;
    WalIterator(std::filesystem::path dir, std::uint64_t from_offset, std::uint64_t safe_offset);

    void load_current();
    bool advance_to_next_record();

    std::filesystem::path dir_;
    std::vector<std::filesystem::path> segment_paths_;
    std::size_t segment_index_ = 0;
    std::uint64_t from_offset_ = 0;
    std::vector<std::uint8_t> segment_bytes_;
    std::uint64_t segment_start_offset_ = 0;
    std::size_t cursor_ = 0;
    bool valid_ = false;
    std::uint64_t current_offset_ = 0;
    BasonRecord current_record_;
    std::uint64_t safe_offset_ = 0;
};

void wal_truncate_before(const std::string& dir, std::uint64_t offset);
