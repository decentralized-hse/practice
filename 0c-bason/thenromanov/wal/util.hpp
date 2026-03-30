#pragma once

#include "wal/constants.hpp"

#include <cstdint>
#include <filesystem>

namespace bason_db {

    std::filesystem::path make_segment_path(const std::filesystem::path& dir,
                                            uint64_t start_offset);

} // namespace bason_db
