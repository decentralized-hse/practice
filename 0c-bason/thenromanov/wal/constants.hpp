#pragma once

#include <cstdint>
#include <string_view>

namespace bason_db {

    constexpr auto kWal = std::string_view{"wal"};

    constexpr auto kWalExtension = std::string_view{".wal"};

    constexpr size_t kHeaderSize = 20;

    constexpr auto kWalTag = std::string_view{"BASONWAL"};
    constexpr uint8_t kCheckpointTag = 0x48; // 'H'

    constexpr uint16_t kVersion = 1;
    constexpr uint16_t kFlags = 0;

} // namespace bason_db
