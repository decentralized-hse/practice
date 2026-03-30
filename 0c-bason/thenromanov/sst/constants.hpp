#pragma once

#include <cstdint>
#include <string_view>

namespace bason_db {

    constexpr auto kSst = std::string_view{"sst"};

    constexpr auto kSstExtension = std::string_view{".sst"};

    constexpr size_t kFooterSize = 48;

    constexpr auto kSstTag = std::string_view{"BASONSST"};

} // namespace bason_db
