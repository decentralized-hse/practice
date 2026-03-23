#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

class BasonRecord {
public:
    BasonRecord() = default;
    explicit BasonRecord(std::vector<std::uint8_t> payload);
    explicit BasonRecord(std::string_view text);

    const std::vector<std::uint8_t>& payload() const noexcept;
    std::size_t payload_size() const noexcept;
    std::vector<std::uint8_t> encode() const;
    void encode_into(std::vector<std::uint8_t>& out) const;
    static std::optional<std::pair<BasonRecord, std::size_t>> decode(std::span<const std::uint8_t> bytes);
    static std::optional<std::size_t> skip(std::span<const std::uint8_t> bytes);

    bool operator==(const BasonRecord& other) const noexcept;
    bool operator!=(const BasonRecord& other) const noexcept;

private:
    std::vector<std::uint8_t> payload_;
};

std::vector<std::uint8_t> encode_u32_le(std::uint32_t value);
std::uint32_t decode_u32_le(const std::uint8_t* bytes);
