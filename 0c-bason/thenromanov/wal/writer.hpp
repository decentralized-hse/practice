#pragma once

#include "codec/record.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>

namespace bason_db {

    class WalWriter {
    public:
        WalWriter() = default;

        WalWriter(const WalWriter&) = delete;
        WalWriter(WalWriter&&) noexcept;

        WalWriter& operator=(const WalWriter&) = delete;
        WalWriter& operator=(WalWriter&&) noexcept;

        ~WalWriter();

        // Open or create a WAL in the given directory.
        static WalWriter open(const std::filesystem::path& dir);

        // Append a BASON record. Returns the global byte offset of the
        // record (its LSN). The record is buffered in memory until
        // checkpoint() or sync() is called.
        uint64_t append(const BasonRecord& record);

        // Write a BLAKE3 hash checkpoint. All records since the last
        // checkpoint become part of an atomic group.
        void checkpoint();

        // Flush buffers and fsync to disk.
        void sync();

        // Rotate to a new segment file. Called when the current segment
        // exceeds a size threshold.
        void rotate(uint64_t max_segment_size);

        uint64_t current_offset() const;

    private:
        void open_segment(uint64_t start_offset);

        void close_segment();

        std::filesystem::path dir_;
        FILE* file_ = nullptr;

        uint64_t current_offset_ = 0;
        uint64_t segment_start_ = 0;
        uint64_t segment_size_ = 0;
    };

} // namespace bason_db
