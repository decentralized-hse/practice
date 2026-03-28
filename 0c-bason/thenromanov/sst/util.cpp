#include "sst/util.hpp"

#include "sst/constants.hpp"

#include <sstream>

namespace bason_db {

    std::filesystem::path make_sst_path(const std::filesystem::path& dir, uint64_t file_number) {
        auto oss = std::ostringstream{};
        oss << std::setw(20) << std::setfill('0') << file_number << kSstExtension;
        return dir / oss.str();
    }

    std::string encode_internal_key(const std::string& user_key, uint64_t offset) {
        auto inverted = std::numeric_limits<uint64_t>::max() - offset;

        auto result = std::string{};
        result.reserve(user_key.size() + 1 + 8);

        result.append(user_key);

        result.push_back('\0');

        for (int i = 7; i >= 0; --i) {
            result.push_back(static_cast<char>((inverted >> (i * 8)) & 0xFF));
        }

        return result;
    }

    std::pair<std::string, uint64_t> decode_internal_key(const std::string& internal_key) {
        if (internal_key.size() < 9) {
            throw std::runtime_error{"Internal key can not be less that 9 chars"};
        }

        auto separator_pos = internal_key.size() - 9;
        if (internal_key[separator_pos] != '\0') {
            throw std::runtime_error{"Wrong char at separator position"};
        }

        auto offset_start = separator_pos + 1;
        auto user_key = internal_key.substr(0, separator_pos);

        uint64_t inverted = 0;
        for (int i = 0; i < 8; ++i) {
            inverted = (inverted << 8) |
                       static_cast<uint8_t>(internal_key[offset_start + static_cast<size_t>(i)]);
        }

        auto offset = std::numeric_limits<uint64_t>::max() - inverted;
        return {user_key, offset};
    }

    std::string extract_user_key(const std::string& internal_key) {
        if (internal_key.size() < 9) {
            throw std::runtime_error{"Internal key can not be less that 9 chars"};
        }

        auto separator_pos = internal_key.size() - 9;
        if (internal_key[separator_pos] != '\0') {
            throw std::runtime_error{"Wrong char at separator position"};
        }

        return internal_key.substr(0, separator_pos);
    }

} // namespace bason_db
