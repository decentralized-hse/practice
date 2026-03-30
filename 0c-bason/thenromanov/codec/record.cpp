#include "record.hpp"

#include <cstdint>
#include <expected>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace bason_db {

    namespace {

        struct BasonTagInfo {
            BasonType type = BasonType::Boolean;
            bool is_short = false;
        };

        constexpr char get_tag(BasonType type, bool is_short = true) {
            char base = '\0';
            switch (type) {
                case BasonType::Boolean:
                    base = 'b';
                    break;
                case BasonType::Array:
                    base = 'a';
                    break;
                case BasonType::String:
                    base = 's';
                    break;
                case BasonType::Object:
                    base = 'o';
                    break;
                case BasonType::Number:
                    base = 'n';
                    break;
                default:
                    throw std::invalid_argument("Unknown BasonType");
            }
            return is_short ? base : static_cast<char>(base - ('a' - 'A'));
        }

        constexpr BasonTagInfo parse_tag(char tag) {
            bool is_short = false;
            BasonType type;
            switch (tag) {
                case 'b':
                    is_short = true;
                    [[fallthrough]];
                case 'B':
                    type = BasonType::Boolean;
                    break;

                case 'a':
                    is_short = true;
                    [[fallthrough]];
                case 'A':
                    type = BasonType::Array;
                    break;

                case 's':
                    is_short = true;
                    [[fallthrough]];
                case 'S':
                    type = BasonType::String;
                    break;

                case 'o':
                    is_short = true;
                    [[fallthrough]];
                case 'O':
                    type = BasonType::Object;
                    break;

                case 'n':
                    is_short = true;
                    [[fallthrough]];
                case 'N':
                    type = BasonType::Number;
                    break;

                default:
                    throw std::invalid_argument("Invalid BASON tag");
            }

            return BasonTagInfo{
                .type = type,
                .is_short = is_short,
            };
        }

    } // namespace

    std::vector<uint8_t> bason_encode(const BasonRecord& record) {
        auto payload = std::vector<uint8_t>{};

        if (record.type == BasonType::Array || record.type == BasonType::Object) {
            for (const auto& child : record.children) {
                payload.append_range(bason_encode(child));
            }
        } else {
            payload.append_range(record.value);
        }

        size_t k_len = record.key.size();
        size_t v_len = payload.size();

        if (k_len > 255) {
            throw std::length_error("BASON key length exceeds 255 bytes");
        }
        if (v_len > 0xFFFFFFFF) {
            throw std::length_error("BASON value length exceeds 4GB");
        }

        bool is_short = (k_len <= 15 && v_len <= 15);

        auto result = std::vector<uint8_t>{};
        result.reserve(1 + (is_short ? 1 : 5) + k_len + v_len);

        result.emplace_back(get_tag(record.type, is_short));

        if (is_short) {
            auto lengths = static_cast<uint8_t>((k_len << 4) | (v_len & 0x0F));
            result.emplace_back(lengths);
        } else {
            auto val_len_le = static_cast<uint32_t>(v_len);
            result.emplace_back(val_len_le & 0xFF);
            result.emplace_back((val_len_le >> 8) & 0xFF);
            result.emplace_back((val_len_le >> 16) & 0xFF);
            result.emplace_back((val_len_le >> 24) & 0xFF);
            result.emplace_back(static_cast<uint8_t>(k_len));
        }

        result.append_range(record.key);
        result.append_range(payload);

        return result;
    }

    std::vector<BasonRecord> bason_decode_all(const uint8_t* data, size_t len) {
        auto records = std::vector<BasonRecord>{};
        size_t offset = 0;

        while (offset < len) {
            auto [record, consumed] = bason_decode(data + offset, len - offset);
            records.emplace_back(std::move(record));
            offset += consumed;
        }

        return records;
    }

    std::pair<BasonRecord, size_t> bason_decode(const uint8_t* data, size_t len) {
        if (len < 2) {
            throw std::runtime_error{"Buffer too small for BASON record"};
        }

        auto tag_info = parse_tag(static_cast<char>(data[0]));

        auto type = tag_info.type;
        bool is_short = tag_info.is_short;

        size_t header_size = is_short ? 2 : 6;
        if (len < header_size) {
            throw std::runtime_error{"Buffer too small for header"};
        }

        size_t k_len = 0;
        size_t v_len = 0;

        if (is_short) {
            k_len = (data[1] >> 4) & 0x0F;
            v_len = data[1] & 0x0F;
        } else {
            v_len = static_cast<uint32_t>(data[1]) | (static_cast<uint32_t>(data[2]) << 8) |
                    (static_cast<uint32_t>(data[3]) << 16) | (static_cast<uint32_t>(data[4]) << 24);
            k_len = data[5];
        }

        size_t total_size = header_size + k_len + v_len;
        if (len < total_size) {
            throw std::runtime_error{"Unexpected end of buffer"};
        }

        auto record = BasonRecord{};
        record.type = type;

        const uint8_t* key_ptr = data + header_size;
        record.key = std::string(reinterpret_cast<const char*>(key_ptr), k_len);

        const uint8_t* val_ptr = key_ptr + k_len;

        if (type == BasonType::Array || type == BasonType::Object) {
            record.children = bason_decode_all(val_ptr, v_len);
        } else {
            record.value = std::string(reinterpret_cast<const char*>(val_ptr), v_len);
        }

        return {std::move(record), total_size};
    }

} // namespace bason_db
