#include "wal.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <blake3.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

constexpr std::size_t kSegmentHeaderSize = 20;
constexpr std::size_t kCheckpointSize = 33;
constexpr std::uint16_t kVersion = 1;
constexpr std::uint16_t kFlags = 0;

constexpr std::array<std::uint8_t, 8> kMagic = {'B', 'A', 'S', 'O', 'N', 'W', 'A', 'L'};
constexpr std::uint8_t kCheckpointTag = static_cast<std::uint8_t>('H');

struct SegmentInfo {
    std::uint64_t start_offset = 0;
    fs::path path;
    std::uint64_t file_size = 0;    
    std::uint64_t logical_size = 0;  
};

[[noreturn]] void throw_system_error(const std::string& what) {
    throw std::system_error(errno, std::generic_category(), what);
}

void store_u16_le(std::uint8_t* dst, std::uint16_t value) {
    dst[0] = static_cast<std::uint8_t>(value & 0xFFu);
    dst[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
}

void store_u64_le(std::uint8_t* dst, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        dst[i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu);
    }
}

std::uint16_t decode_u16_le(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8);
}

std::uint64_t decode_u64_le(const std::uint8_t* p) {
    std::uint64_t v = 0;
    for (int i = 7; i >= 0; --i) {
        v <<= 8;
        v |= p[i];
    }
    return v;
}

std::array<std::uint8_t, kSegmentHeaderSize> make_header(std::uint64_t start_offset) {
    std::array<std::uint8_t, kSegmentHeaderSize> header{};
    std::memcpy(header.data(), kMagic.data(), kMagic.size());
    store_u16_le(header.data() + 8, kVersion);
    store_u16_le(header.data() + 10, kFlags);
    store_u64_le(header.data() + 12, start_offset);
    return header;
}

bool parse_header(std::span<const std::uint8_t> bytes, std::uint64_t& start_offset) {
    if (bytes.size() < kSegmentHeaderSize) {
        return false;
    }
    for (std::size_t i = 0; i < kMagic.size(); ++i) {
        if (bytes[i] != kMagic[i]) {
            return false;
        }
    }
    const auto version = decode_u16_le(bytes.data() + 8);
    const auto flags = decode_u16_le(bytes.data() + 10);
    (void)flags;
    if (version != kVersion) {
        return false;
    }
    start_offset = decode_u64_le(bytes.data() + 12);
    return true;
}

std::string segment_filename(std::uint64_t start_offset) {
    std::ostringstream oss;
    oss << std::setw(20) << std::setfill('0') << start_offset << ".wal";
    return oss.str();
}

bool is_segment_name(const std::string& name) {
    static const std::regex re(R"(^\d{20}\.wal$)");
    return std::regex_match(name, re);
}

std::uint64_t parse_segment_name(const std::string& name) {
    return static_cast<std::uint64_t>(std::stoull(name.substr(0, 20)));
}

std::vector<SegmentInfo> list_segments(const fs::path& dir) {
    std::vector<SegmentInfo> segments;
    if (!fs::exists(dir)) {
        return segments;
    }

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto filename = entry.path().filename().string();
        if (!is_segment_name(filename)) {
            continue;
        }

        SegmentInfo info;
        info.start_offset = parse_segment_name(filename);
        info.path = entry.path();
        info.file_size = static_cast<std::uint64_t>(fs::file_size(entry.path()));
        info.logical_size = info.file_size >= kSegmentHeaderSize ? info.file_size - kSegmentHeaderSize : 0;
        segments.push_back(std::move(info));
    }

    std::sort(segments.begin(), segments.end(),
              [](const SegmentInfo& a, const SegmentInfo& b) { return a.start_offset < b.start_offset; });
    return segments;
}

struct MappedFile {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;

    MappedFile() = default;
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    MappedFile(MappedFile&& o) noexcept : data(o.data), size(o.size) { o.data = nullptr; o.size = 0; }

    MappedFile& operator=(MappedFile&& o) noexcept {
        if (this != &o) {
            if (data && size) {
                ::munmap(const_cast<std::uint8_t*>(data), size);
            }
            data = o.data;
            size = o.size;
            o.data = nullptr;
            o.size = 0;
        }
        return *this;
    }

    ~MappedFile() {
        if (data && size) {
            ::munmap(const_cast<std::uint8_t*>(data), size);
        }
    }

    static MappedFile open(const fs::path& path) {
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) throw_system_error("open for mmap");

        struct stat st{};
        if (::fstat(fd, &st) != 0) { ::close(fd); throw_system_error("fstat"); }

        MappedFile mf;
        mf.size = static_cast<std::size_t>(st.st_size);
        if (mf.size == 0) { ::close(fd); return mf; }

        void* ptr = ::mmap(nullptr, mf.size, PROT_READ, MAP_PRIVATE, fd, 0);
        ::close(fd);
        if (ptr == MAP_FAILED) throw_system_error("mmap");
        ::madvise(ptr, mf.size, MADV_SEQUENTIAL);
        mf.data = static_cast<const std::uint8_t*>(ptr);
        return mf;
    }
};

void write_all(int fd, const std::uint8_t* data, std::size_t len) {
    std::size_t written = 0;
    while (written < len) {
        ssize_t rc = ::write(fd, data + written, len - written);
        if (rc < 0) {
            if (errno == EINTR) continue;
            throw_system_error("write");
        }
        written += static_cast<std::size_t>(rc);
    }
}

void write_all(int fd, const std::vector<std::uint8_t>& data) {
    if (!data.empty()) {
        write_all(fd, data.data(), data.size());
    }
}

void fsync_directory(const fs::path& dir) {
    int dfd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
    if (dfd < 0) {
        throw_system_error("open directory");
    }
    if (::fsync(dfd) != 0) {
        int saved = errno;
        ::close(dfd);
        errno = saved;
        throw_system_error("fsync directory");
    }
    ::close(dfd);
}

std::size_t padding_size(std::size_t size) {
    return (8 - (size % 8)) % 8;
}

void append_record_to_pending(std::vector<std::uint8_t>& pending, const BasonRecord& record) {
    const std::size_t before = pending.size();
    if (record.type == BasonType::String && record.key.empty() && record.children.empty()) {
        pending.reserve(before + 5 + record.value.size() + 7);
    }
    bason_encode_into(record, pending);
    const std::size_t rec_len = pending.size() - before;
    const std::size_t pad = padding_size(rec_len);
    if (pad != 0) {
        pending.resize(pending.size() + pad, 0);
    }
}

std::uint64_t logical_end_offset(const SegmentInfo& s) {
    return s.start_offset + s.logical_size;
}

bool env_disable_fsync() {
#ifdef WAL_DISABLE_FSYNC
    return true;
#else
    const char* env = std::getenv("WAL_DISABLE_FSYNC");
    return env != nullptr && std::string_view(env) == "1";
#endif
}

struct RecoveryResult {
    std::uint64_t recover_offset = 0;
    std::size_t truncate_segment_index = 0;
    bool has_truncate_segment = false;
    bool delete_from_next_segment = false;
};

RecoveryResult recover_impl(const fs::path& dir, const std::vector<SegmentInfo>& segments) {
    RecoveryResult result{};
    if (segments.empty()) {
        result.recover_offset = 0;
        return result;
    }

    result.recover_offset = segments.front().start_offset;

    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];
        auto mapped = MappedFile::open(seg.path);
        const auto* file_data = mapped.data;
        const auto file_size = mapped.size;

        std::uint64_t header_start = 0;
        if (!parse_header(std::span<const std::uint8_t>(file_data, file_size), header_start)) {
            result.truncate_segment_index = i;
            result.has_truncate_segment = true;
            result.delete_from_next_segment = true;
            return result;
        }

        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, file_data, kSegmentHeaderSize);

        std::size_t pos = kSegmentHeaderSize;
        std::size_t hash_run_start = kSegmentHeaderSize;
        std::uint64_t logical_pos = seg.start_offset;
        std::uint64_t last_good_checkpoint = result.recover_offset;

        while (pos < file_size) {
            if (file_data[pos] == kCheckpointTag) {
                if (pos > hash_run_start) {
                    blake3_hasher_update(&hasher, file_data + hash_run_start, pos - hash_run_start);
                }
                if (file_size - pos < kCheckpointSize) {
                    result.recover_offset = last_good_checkpoint;
                    result.truncate_segment_index = i;
                    result.has_truncate_segment = true;
                    result.delete_from_next_segment = true;
                    return result;
                }

                blake3_hasher checkpoint_hasher = hasher;
                std::uint8_t digest[32];
                blake3_hasher_update(&checkpoint_hasher, &file_data[pos], 1);
                blake3_hasher_finalize(&checkpoint_hasher, digest, sizeof(digest));

                if (std::memcmp(digest, &file_data[pos + 1], 32) != 0) {
                    result.recover_offset = last_good_checkpoint;
                    result.truncate_segment_index = i;
                    result.has_truncate_segment = true;
                    result.delete_from_next_segment = true;
                    return result;
                }

                blake3_hasher_update(&hasher, &file_data[pos], 1);
                pos += kCheckpointSize;
                logical_pos += kCheckpointSize;
                last_good_checkpoint = logical_pos;
                result.recover_offset = last_good_checkpoint;
                hash_run_start = pos;
                continue;
            }

            auto skipped = bason_skip_record(std::span<const std::uint8_t>(file_data + pos, file_size - pos));
            if (!skipped.has_value()) {
                result.recover_offset = last_good_checkpoint;
                result.truncate_segment_index = i;
                result.has_truncate_segment = true;
                result.delete_from_next_segment = true;
                return result;
            }

            const std::size_t consumed = skipped.value();
            const std::size_t padded = consumed + padding_size(consumed);
            if (pos + padded > file_size) {
                result.recover_offset = last_good_checkpoint;
                result.truncate_segment_index = i;
                result.has_truncate_segment = true;
                result.delete_from_next_segment = true;
                return result;
            }
            for (std::size_t j = consumed; j < padded; ++j) {
                if (file_data[pos + j] != 0) {
                    result.recover_offset = last_good_checkpoint;
                    result.truncate_segment_index = i;
                    result.has_truncate_segment = true;
                    result.delete_from_next_segment = true;
                    return result;
                }
            }

            pos += padded;
            logical_pos += padded;
        }

        if (pos > hash_run_start) {
            blake3_hasher_update(&hasher, file_data + hash_run_start, pos - hash_run_start);
        }
    }

    return result;
}

void truncate_and_cleanup(const fs::path& dir,
                          const std::vector<SegmentInfo>& segments,
                          const RecoveryResult& result) {
    if (segments.empty()) {
        return;
    }

    if (!result.has_truncate_segment) {
        return;
    }

    const auto& seg = segments[result.truncate_segment_index];
    const std::uint64_t keep_up_to = result.recover_offset;
    const std::uint64_t file_keep = keep_up_to <= seg.start_offset
        ? kSegmentHeaderSize
        : kSegmentHeaderSize + (keep_up_to - seg.start_offset);

    if (::truncate(seg.path.c_str(), static_cast<off_t>(file_keep)) != 0) {
        throw_system_error("truncate");
    }

    for (std::size_t i = result.truncate_segment_index + 1; i < segments.size(); ++i) {
        ::unlink(segments[i].path.c_str());
    }

    fsync_directory(dir);
}

std::uint64_t scan_segment_for_recovery(const fs::path& dir, const std::vector<SegmentInfo>& segments) {
    if (segments.empty()) {
        return 0;
    }
    auto result = recover_impl(dir, segments);
    truncate_and_cleanup(dir, segments, result);
    return result.recover_offset;
}

void ensure_directory_exists(const fs::path& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        throw std::system_error(ec);
    }
}

} 

struct WalWriter::Blake3State {
    blake3_hasher hasher;
};

WalWriter::WalWriter(WalWriter&& other) noexcept
    : dir_(std::move(other.dir_)),
      current_path_(std::move(other.current_path_)),
      current_segment_start_offset_(other.current_segment_start_offset_),
      current_segment_logical_size_(other.current_segment_logical_size_),
      committed_stream_offset_(other.committed_stream_offset_),
      pending_(std::move(other.pending_)),
      fd_(other.fd_),
      last_sync_offset_(other.last_sync_offset_),
      hasher_(other.hasher_) {
    other.fd_ = -1;
    other.hasher_ = nullptr;
}

WalWriter& WalWriter::operator=(WalWriter&& other) noexcept {
    if (this == &other) return *this;
    close_current_segment();
    dir_ = std::move(other.dir_);
    current_path_ = std::move(other.current_path_);
    current_segment_start_offset_ = other.current_segment_start_offset_;
    current_segment_logical_size_ = other.current_segment_logical_size_;
    committed_stream_offset_ = other.committed_stream_offset_;
    pending_ = std::move(other.pending_);
    fd_ = other.fd_;
    last_sync_offset_ = other.last_sync_offset_;
    hasher_ = other.hasher_;
    other.fd_ = -1;
    other.hasher_ = nullptr;
    return *this;
}

WalWriter::~WalWriter() {
    close_current_segment();
}

void WalWriter::close_current_segment() noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    delete hasher_;
    hasher_ = nullptr;
}

void WalWriter::update_hasher_with_file_bytes_after_header() {
    if (!hasher_) {
        hasher_ = new Blake3State();
    }
    blake3_hasher_init(&hasher_->hasher);

    struct stat st {};
    if (::fstat(fd_, &st) != 0) {
        throw_system_error("fstat segment for hasher");
    }
    const auto file_size = static_cast<std::size_t>(st.st_size);
    if (file_size < kSegmentHeaderSize) {
        throw std::runtime_error("segment is too small: " + current_path_.string());
    }
    void* mapped =
        ::mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mapped == MAP_FAILED) {
        throw_system_error("mmap segment for hasher");
    }
    blake3_hasher_update(&hasher_->hasher, mapped, file_size);
    ::munmap(mapped, file_size);
}

void WalWriter::open_segment(std::uint64_t start_offset, bool create_new) {
    current_segment_start_offset_ = start_offset;
    current_segment_logical_size_ = 0;
    committed_stream_offset_ = start_offset;
    pending_.clear();
    pending_.reserve(std::min<std::size_t>(kFlushThreshold * 2, 16384));

    current_path_ = dir_ / segment_filename(start_offset);
    const auto header = make_header(start_offset);

    int flags = O_RDWR | O_CREAT;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    if (!create_new) {
        flags = O_RDWR;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
    }

    fd_ = ::open(current_path_.c_str(), flags, 0644);
    if (fd_ < 0) {
        throw_system_error("open segment");
    }

    struct stat st {};
    if (::fstat(fd_, &st) != 0) {
        int saved = errno;
        ::close(fd_);
        fd_ = -1;
        errno = saved;
        throw_system_error("fstat");
    }

    if (st.st_size == 0) {
        write_all(fd_, header.data(), header.size());
        if (::fsync(fd_) != 0) {
            int saved = errno;
            ::close(fd_);
            fd_ = -1;
            errno = saved;
            throw_system_error("fsync");
        }
        fsync_directory(dir_);
    } else {
        std::array<std::uint8_t, kSegmentHeaderSize> hdr{};
        const ssize_t n = ::pread(fd_, hdr.data(), hdr.size(), 0);
        if (n != static_cast<ssize_t>(kSegmentHeaderSize)) {
            throw std::runtime_error("pread WAL segment header: " + current_path_.string());
        }
        std::uint64_t parsed_start = 0;
        if (!parse_header(std::span<const std::uint8_t>(hdr.data(), hdr.size()), parsed_start)) {
            throw std::runtime_error("invalid existing WAL segment header: " + current_path_.string());
        }
        if (parsed_start != start_offset) {
            throw std::runtime_error("segment start_offset mismatch: " + current_path_.string());
        }
    }

    hasher_ = new Blake3State();
    update_hasher_with_file_bytes_after_header();

    current_segment_logical_size_ = static_cast<std::uint64_t>(fs::file_size(current_path_)) - kSegmentHeaderSize;
    committed_stream_offset_ = current_segment_start_offset_ + current_segment_logical_size_;

    if (::lseek(fd_, 0, SEEK_END) < 0) {
        throw_system_error("lseek");
    }
}

WalWriter WalWriter::open(const std::string& dir) {
    WalWriter writer;
    writer.dir_ = dir;
    ensure_directory_exists(writer.dir_);
    std::vector<SegmentInfo> segments = list_segments(writer.dir_);
    if (!segments.empty()) {
        (void)scan_segment_for_recovery(writer.dir_, segments);
    }

    segments = list_segments(writer.dir_);
    if (segments.empty()) {
        writer.open_segment(0, true);
    } else {
        const auto& last = segments.back();
        writer.open_segment(last.start_offset, false);
    }
    return writer;
}

std::uint64_t WalWriter::append(const BasonRecord& record) {
    const std::uint64_t offset = committed_stream_offset_ + pending_.size();
    append_record_to_pending(pending_, record);
    flush_pending_to_disk();
    return offset;
}

uint64_t WalWriter::append_buffered(const BasonRecord& record) {
    const std::uint64_t offset = committed_stream_offset_ + pending_.size();
    append_record_to_pending(pending_, record);
    if (pending_.size() >= kFlushThreshold) {
        flush_pending_to_disk();
    }
    return offset;
}

void WalWriter::flush_pending_to_disk() {
    if (pending_.empty()) return;
    write_all(fd_, pending_);
    blake3_hasher_update(&hasher_->hasher, pending_.data(), pending_.size());

    current_segment_logical_size_ += pending_.size();
    committed_stream_offset_ = current_segment_start_offset_ + current_segment_logical_size_;
    pending_.clear();
}

void WalWriter::checkpoint() {
    flush_pending_to_disk();

    blake3_hasher tmp = hasher_->hasher;
    std::uint8_t buf[kCheckpointSize];
    buf[0] = kCheckpointTag;
    blake3_hasher_update(&tmp, buf, 1);
    blake3_hasher_finalize(&tmp, buf + 1, 32);
    write_all(fd_, buf, kCheckpointSize);
    blake3_hasher_update(&hasher_->hasher, buf, 1);
    current_segment_logical_size_ += kCheckpointSize;
    committed_stream_offset_ = current_segment_start_offset_ + current_segment_logical_size_;
}

void WalWriter::sync() {
    flush_pending_to_disk();
    if (env_disable_fsync()) {
        return;
    }
    if (fd_ >= 0 && ::fsync(fd_) != 0) {
        throw_system_error("fsync");
    }
}

void WalWriter::sync_periodic(bool force) {
    flush_pending_to_disk();

    const std::uint64_t current_total = current_segment_start_offset_ + current_segment_logical_size_;
    const std::uint64_t bytes_since = current_total - last_sync_offset_;

    if (force || bytes_since >= 32 * 1024 * 1024) {
        if (!env_disable_fsync() && fd_ >= 0 && ::fsync(fd_) != 0) {
            throw_system_error("fsync");
        }
        last_sync_offset_ = current_total;
    }
}

void WalWriter::rotate(std::uint64_t max_segment_size) {
    flush_pending_to_disk();
    const std::uint64_t future_size = current_segment_logical_size_ + pending_.size();

    if (future_size < max_segment_size) {
        return;
    }

    close_current_segment();
    const std::uint64_t new_start = committed_stream_offset_;
    open_segment(new_start, true);
}

WalReader::WalReader(WalReader&& other) noexcept
    : dir_(std::move(other.dir_)) {}

WalReader& WalReader::operator=(WalReader&& other) noexcept {
    if (this == &other) return *this;
    dir_ = std::move(other.dir_);
    return *this;
}

WalReader::~WalReader() = default;

WalReader WalReader::open(const std::string& dir) {
    WalReader reader;
    reader.dir_ = dir;
    ensure_directory_exists(reader.dir_);
    return reader;
}

std::uint64_t WalReader::recover() {
    const auto segments = list_segments(dir_);
    safe_offset_ = scan_segment_for_recovery(dir_, segments);
    recovered_ = true;
    return safe_offset_;
}

WalIterator WalReader::scan(uint64_t from_offset) {
    if (!recovered_) {
        throw std::runtime_error("WalReader::scan() called before recover()");
    }
    return WalIterator(dir_, from_offset, safe_offset_);
}

WalIterator::WalIterator(WalIterator&& other) noexcept
    : dir_(std::move(other.dir_)),
      segment_paths_(std::move(other.segment_paths_)),
      segment_index_(other.segment_index_),
      from_offset_(other.from_offset_),
      segment_bytes_(std::move(other.segment_bytes_)),
      segment_start_offset_(other.segment_start_offset_),
      cursor_(other.cursor_),
      valid_(other.valid_),
      current_offset_(other.current_offset_),
      current_record_(std::move(other.current_record_)) {
    other.valid_ = false;
}

WalIterator& WalIterator::operator=(WalIterator&& other) noexcept {
    if (this == &other) return *this;
    dir_ = std::move(other.dir_);
    segment_paths_ = std::move(other.segment_paths_);
    segment_index_ = other.segment_index_;
    from_offset_ = other.from_offset_;
    segment_bytes_ = std::move(other.segment_bytes_);
    segment_start_offset_ = other.segment_start_offset_;
    cursor_ = other.cursor_;
    valid_ = other.valid_;
    current_offset_ = other.current_offset_;
    current_record_ = std::move(other.current_record_);
    other.valid_ = false;
    return *this;
}

WalIterator::~WalIterator() = default;

WalIterator::WalIterator(std::filesystem::path dir,
                         std::uint64_t from_offset,
                         std::uint64_t safe_offset)
    : dir_(std::move(dir)),
      from_offset_(from_offset),
      safe_offset_(safe_offset)
{
    const auto segments = list_segments(dir_);
    segment_paths_.reserve(segments.size());
    for (const auto& s : segments) {
        segment_paths_.push_back(s.path);
    }
    if (!segment_paths_.empty()) {
        load_current();
        advance_to_next_record();
    }
}

bool WalIterator::valid() const noexcept {
    return valid_;
}

std::uint64_t WalIterator::offset() const {
    if (!valid_) {
        throw std::runtime_error("WalIterator::offset() called on invalid iterator");
    }
    return current_offset_;
}

const BasonRecord& WalIterator::record() const {
    if (!valid_) {
        throw std::runtime_error("WalIterator::record() called on invalid iterator");
    }
    return current_record_;
}

void WalIterator::load_current() {
    if (segment_index_ >= segment_paths_.size()) {
        segment_bytes_.clear();
        valid_ = false;
        return;
    }

    const auto& path = segment_paths_[segment_index_];
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        throw_system_error("WalIterator open segment");
    }
    struct stat st {};
    if (::fstat(fd, &st) != 0) {
        int saved = errno;
        ::close(fd);
        errno = saved;
        throw_system_error("WalIterator fstat segment");
    }
    const auto sz = static_cast<std::size_t>(st.st_size);
    segment_bytes_.resize(sz);
    if (sz > 0) {
        const ssize_t n = ::pread(fd, segment_bytes_.data(), sz, 0);
        if (n < 0) {
            int saved = errno;
            ::close(fd);
            errno = saved;
            throw_system_error("WalIterator pread segment");
        }
        if (static_cast<std::size_t>(n) != sz) {
            ::close(fd);
            throw std::runtime_error("WalIterator short read: " + path.string());
        }
    }
    ::close(fd);

    if (segment_bytes_.size() < kSegmentHeaderSize) {
        valid_ = false;
        return;
    }
    if (!parse_header(std::span<const std::uint8_t>(segment_bytes_.data(), segment_bytes_.size()), segment_start_offset_)) {
        valid_ = false;
        return;
    }
    cursor_ = kSegmentHeaderSize;
}

bool WalIterator::advance_to_next_record() {
    valid_ = false;

    while (true) {
        if (segment_index_ >= segment_paths_.size()) {
            return false;
        }

        if (segment_bytes_.empty()) {
            load_current();
            if (segment_index_ >= segment_paths_.size() || segment_bytes_.empty()) {
                return false;
            }
        }

        while (cursor_ < segment_bytes_.size()) {
            const std::uint64_t current_global = segment_start_offset_ + (cursor_ - kSegmentHeaderSize);
            if (current_global >= safe_offset_) {
                return false;
            }

            if (segment_bytes_[cursor_] == kCheckpointTag) {
                if (segment_bytes_.size() - cursor_ < kCheckpointSize) {
                    return false;
                }
                cursor_ += kCheckpointSize;
                continue;
            }

            auto decoded = bason_try_decode(std::span<const std::uint8_t>(segment_bytes_.data() + cursor_,
                                                                         segment_bytes_.size() - cursor_));
            if (!decoded.has_value()) {
                return false;
            }

            auto [record, consumed] = *decoded;
            const std::size_t padded = consumed + padding_size(consumed);
            if (cursor_ + padded > segment_bytes_.size()) {
                return false;
            }
            for (std::size_t j = consumed; j < padded; ++j) {
                if (segment_bytes_[cursor_ + j] != 0) {
                    return false;
                }
            }

            current_offset_ = current_global;
            current_record_ = std::move(record);
            cursor_ += padded;

            if (current_offset_ >= from_offset_) {
                valid_ = true;
                return true;
            }
        }

        ++segment_index_;
        segment_bytes_.clear();
        cursor_ = 0;
        if (segment_index_ < segment_paths_.size()) {
            load_current();
            if (segment_index_ >= segment_paths_.size() || segment_bytes_.empty()) {
                return false;
            }
        } else {
            return false;
        }
    }
}

void WalIterator::next() {
    if (!valid_) {
        return;
    }
    if (!advance_to_next_record()) {
        valid_ = false;
    }
}

void wal_truncate_before(const std::string& dir, std::uint64_t offset) {
    fs::path root(dir);
    ensure_directory_exists(root);
    auto segments = list_segments(root);

    bool deleted_any = false;
    for (const auto& seg : segments) {
        if (logical_end_offset(seg) <= offset) {
            ::unlink(seg.path.c_str());
            deleted_any = true;
        }
    }

    if (deleted_any) {
        fsync_directory(root);
    }
}
