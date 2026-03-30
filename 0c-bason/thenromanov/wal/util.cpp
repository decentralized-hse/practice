#include "wal/util.hpp"

#include "wal/constants.hpp"

#include <sstream>

namespace bason_db {

    std::filesystem::path make_segment_path(const std::filesystem::path& dir,
                                            uint64_t start_offset) {
        auto oss = std::ostringstream{};
        oss << std::setw(20) << std::setfill('0') << start_offset << kWalExtension;
        return dir / oss.str();
    }

} // namespace bason_db
