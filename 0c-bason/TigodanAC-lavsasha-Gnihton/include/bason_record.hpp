#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// --- Data model (Assignment 1 public API) ---------------------------------

enum class BasonType { Boolean, Array, String, Object, Number };

struct BasonRecord {
    BasonType type = BasonType::String;
    std::string key;
    std::string value;
    std::vector<BasonRecord> children;

    bool operator==(const BasonRecord& other) const noexcept;
    bool operator!=(const BasonRecord& other) const noexcept;

    /// String leaf with empty key — удобство для тестов WAL до появления путей из RFC.
    static BasonRecord leaf_string(std::string_view utf8_value);
};

// --- Codec (Assignment 1): сейчас заглушка; заменить реализацию в bason_record.cpp

std::vector<std::uint8_t> bason_encode(const BasonRecord& record);
void bason_encode_into(const BasonRecord& record, std::vector<std::uint8_t>& out);

std::pair<BasonRecord, std::size_t> bason_decode(const std::uint8_t* data, std::size_t len);

/// Для WAL / skip без исключений.
std::optional<std::pair<BasonRecord, std::size_t>> bason_try_decode(std::span<const std::uint8_t> bytes);
std::optional<std::size_t> bason_skip_record(std::span<const std::uint8_t> bytes);
