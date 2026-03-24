#include "bason_record.hpp"

#include <limits>
#include <stdexcept>

namespace {

// После интеграции Assignment 1 этот файл заменяют на настоящий TLV-кодек;
// WAL продолжает писать ровно то, что возвращает bason_encode().
constexpr std::uint8_t kStubWireTag = 0x01;
constexpr std::size_t kStubHeaderSize = 1 + 4;

void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
}

std::uint32_t read_u32_le(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

}  // namespace

bool BasonRecord::operator==(const BasonRecord& other) const noexcept {
    return type == other.type && key == other.key && value == other.value && children == other.children;
}

bool BasonRecord::operator!=(const BasonRecord& other) const noexcept {
    return !(*this == other);
}

BasonRecord BasonRecord::leaf_string(std::string_view utf8_value) {
    BasonRecord r;
    r.type = BasonType::String;
    r.value = std::string(utf8_value);
    return r;
}

std::vector<std::uint8_t> bason_encode(const BasonRecord& record) {
    std::vector<std::uint8_t> out;
    bason_encode_into(record, out);
    return out;
}

void bason_encode_into(const BasonRecord& record, std::vector<std::uint8_t>& out) {
    // Заглушка: только лист String с пустым key (типичный upsert значения по умолчанию).
    if (record.type != BasonType::String || !record.key.empty() || !record.children.empty()) {
        throw std::runtime_error(
            "bason_encode (stub): only String leaf with empty key and no children; "
            "integrate Assignment 1 codec for full BASON.");
    }
    if (record.value.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        throw std::runtime_error("bason_encode (stub): value too large");
    }
    out.push_back(kStubWireTag);
    append_u32_le(out, static_cast<std::uint32_t>(record.value.size()));
    out.insert(out.end(), record.value.begin(), record.value.end());
}

std::optional<std::pair<BasonRecord, std::size_t>> bason_try_decode(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kStubHeaderSize) {
        return std::nullopt;
    }
    if (bytes[0] != kStubWireTag) {
        return std::nullopt;
    }
    const std::uint32_t len = read_u32_le(bytes.data() + 1);
    if (bytes.size() < kStubHeaderSize + len) {
        return std::nullopt;
    }
    BasonRecord rec;
    rec.type = BasonType::String;
    rec.key.clear();
    rec.value.assign(bytes.begin() + static_cast<std::ptrdiff_t>(kStubHeaderSize),
                     bytes.begin() + static_cast<std::ptrdiff_t>(kStubHeaderSize + len));
    return std::make_pair(std::move(rec), kStubHeaderSize + static_cast<std::size_t>(len));
}

std::pair<BasonRecord, std::size_t> bason_decode(const std::uint8_t* data, std::size_t len) {
    auto r = bason_try_decode(std::span<const std::uint8_t>(data, len));
    if (!r) {
        throw std::runtime_error("malformed BASON record (stub codec)");
    }
    return *r;
}

std::optional<std::size_t> bason_skip_record(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kStubHeaderSize) {
        return std::nullopt;
    }
    if (bytes[0] != kStubWireTag) {
        return std::nullopt;
    }
    const std::uint32_t len = read_u32_le(bytes.data() + 1);
    if (bytes.size() < kStubHeaderSize + len) {
        return std::nullopt;
    }
    return kStubHeaderSize + static_cast<std::size_t>(len);
}
