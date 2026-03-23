#include "bason_record.hpp"

#include <cstring>

namespace {
constexpr std::uint8_t kTag = 0x01;
constexpr std::size_t kHeaderSize = 1 + 4;
}

BasonRecord::BasonRecord(std::vector<std::uint8_t> payload)
    : payload_(std::move(payload)) {}

BasonRecord::BasonRecord(std::string_view text)
    : payload_(text.begin(), text.end()) {}

const std::vector<std::uint8_t>& BasonRecord::payload() const noexcept {
    return payload_;
}

std::size_t BasonRecord::payload_size() const noexcept {
    return payload_.size();
}

std::vector<std::uint8_t> encode_u32_le(std::uint32_t value) {
    std::vector<std::uint8_t> out(4);
    out[0] = static_cast<std::uint8_t>(value & 0xFFu);
    out[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    out[2] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
    out[3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
    return out;
}

std::uint32_t decode_u32_le(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::vector<std::uint8_t> BasonRecord::encode() const {
    std::vector<std::uint8_t> out;
    out.reserve(kHeaderSize + payload_.size());
    encode_into(out);
    return out;
}

void BasonRecord::encode_into(std::vector<std::uint8_t>& out) const {
    out.push_back(kTag);
    const auto sz = static_cast<std::uint32_t>(payload_.size());
    const std::uint8_t len_bytes[4] = {
        static_cast<std::uint8_t>(sz & 0xFFu),
        static_cast<std::uint8_t>((sz >> 8) & 0xFFu),
        static_cast<std::uint8_t>((sz >> 16) & 0xFFu),
        static_cast<std::uint8_t>((sz >> 24) & 0xFFu),
    };
    out.insert(out.end(), len_bytes, len_bytes + 4);
    out.insert(out.end(), payload_.begin(), payload_.end());
}

std::optional<std::pair<BasonRecord, std::size_t>> BasonRecord::decode(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kHeaderSize) {
        return std::nullopt;
    }
    if (bytes[0] != kTag) {
        return std::nullopt;
    }
    std::uint32_t len = decode_u32_le(bytes.data() + 1);
    if (bytes.size() < kHeaderSize + len) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> payload(bytes.begin() + static_cast<std::ptrdiff_t>(kHeaderSize),
                                      bytes.begin() + static_cast<std::ptrdiff_t>(kHeaderSize + len));
    return std::make_optional(std::make_pair(BasonRecord(std::move(payload)), kHeaderSize + len));
}

std::optional<std::size_t> BasonRecord::skip(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kHeaderSize) {
        return std::nullopt;
    }
    if (bytes[0] != kTag) {
        return std::nullopt;
    }
    std::uint32_t len = decode_u32_le(bytes.data() + 1);
    if (bytes.size() < kHeaderSize + len) {
        return std::nullopt;
    }
    return kHeaderSize + len;
}

bool BasonRecord::operator==(const BasonRecord& other) const noexcept {
    return payload_ == other.payload_;
}

bool BasonRecord::operator!=(const BasonRecord& other) const noexcept {
    return !(*this == other);
}
