#pragma once

#include "sst/constants.hpp"

#include <cstdint>
#include <filesystem>

namespace bason_db {

    std::filesystem::path make_sst_path(const std::filesystem::path& dir, uint64_t file_number);

} // namespace bason_db
