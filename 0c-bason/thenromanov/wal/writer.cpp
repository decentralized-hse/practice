#include "wal/writer.hpp"

#include "codec/record.hpp"

#include "wal/constants.hpp"
#include "wal/util.hpp"

#include <cstdio>
#include <unistd.h>

namespace bason_db {

    namespace {

        void write_u16_le(FILE* f, uint16_t v) {
            std::fwrite(&v, 2, 1, f);
        }

        void write_u64_le(FILE* f, uint64_t v) {
            std::fwrite(&v, 8, 1, f);
        }

    } // namespace

    WalWriter::WalWriter(WalWriter&& other) noexcept
        : dir_{std::move(other.dir_)}
        , file_{other.file_}
        , current_offset_{other.current_offset_}
        , segment_start_{other.segment_start_}
        , segment_size_{other.segment_size_} {
        other.file_ = nullptr;
    }

    WalWriter& WalWriter::operator=(WalWriter&& other) noexcept {
        if (this != &other) {
            close_segment();
            dir_ = std::move(other.dir_);
            file_ = other.file_;
            current_offset_ = other.current_offset_;
            segment_start_ = other.segment_start_;
            segment_size_ = other.segment_size_;
            other.file_ = nullptr;
        }
        return *this;
    }

    WalWriter::~WalWriter() {
        close_segment();
    }

    WalWriter WalWriter::open(const std::filesystem::path& dir) {
        std::filesystem::create_directories(dir);

        auto writer = WalWriter{};
        writer.dir_ = dir;

        uint64_t max_offset = 0;
        bool found = false;
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == kWalExtension) {
                try {
                    uint64_t off = std::stoull(entry.path().stem().string());
                    if (!found || off > max_offset) {
                        max_offset = off;
                        found = true;
                    }
                } catch (...) {
                }
            }
        }

        if (found) {
            auto path = make_segment_path(dir, max_offset);
            uint64_t fsize = std::filesystem::file_size(path);
            writer.current_offset_ = max_offset + fsize;
            writer.segment_start_ = max_offset;
            writer.segment_size_ = fsize;
            writer.file_ = std::fopen(path.string().c_str(), "ab");
            if (writer.file_ == nullptr) {
                throw std::runtime_error{"Cannot open existing segment: " + path.string()};
            }
        } else {
            writer.open_segment(0);
        }

        return writer;
    }

    uint64_t WalWriter::append(const BasonRecord& record) {
        auto bytes = bason_encode(record);

        size_t padded = (bytes.size() + 7) & ~7ULL;
        bytes.resize(padded, 0);

        uint64_t record_offset = current_offset_;

        std::fwrite(bytes.data(), 1, bytes.size(), file_);

        current_offset_ += bytes.size();
        segment_size_ += bytes.size();

        return record_offset;
    }

    void WalWriter::checkpoint() {
        uint8_t marker = kCheckpointTag;
        std::fwrite(&marker, 1, 1, file_);

        uint32_t crc = 0;
        std::fwrite(&crc, 4, 1, file_);

        current_offset_ += 5;
        segment_size_ += 5;
    }

    void WalWriter::sync() {
        if (file_ == nullptr) {
            return;
        }

        if (std::fflush(file_) != 0) {
            throw std::runtime_error{"Error in fflush: " + std::string{strerror(errno)}};
        }

        int fd = fileno(file_);
        if (fd == -1) {
            throw std::runtime_error{"Error getting file descriptor"};
        }

        if (fsync(fd) == -1) {
            throw std::runtime_error{"Error in fsync: " + std::string{strerror(errno)}};
        }
    }

    void WalWriter::rotate(uint64_t max_segment_size) {
        if (segment_size_ >= max_segment_size) {
            close_segment();
            open_segment(current_offset_);
        }
    }

    uint64_t WalWriter::current_offset() const {
        return current_offset_;
    }

    void WalWriter::open_segment(uint64_t start_offset) {
        segment_start_ = start_offset;
        segment_size_ = 0;

        auto path = make_segment_path(dir_, start_offset);
        file_ = std::fopen(path.string().c_str(), "ab");
        if (file_ == nullptr) {
            throw std::runtime_error{"Cannot open segment file: " + path.string()};
        }

        if (std::filesystem::file_size(path) == 0) {
            std::fwrite(kWalTag.data(), 1, kWalTag.size(), file_);
            write_u16_le(file_, kVersion);
            write_u16_le(file_, kFlags);
            write_u64_le(file_, start_offset);
            segment_size_ = kHeaderSize;
        }

        current_offset_ = segment_start_ + segment_size_;
    }

    void WalWriter::close_segment() {
        if (file_ != nullptr) {
            std::fflush(file_);
            std::fclose(file_);
            file_ = nullptr;
        }
    }

} // namespace bason_db
