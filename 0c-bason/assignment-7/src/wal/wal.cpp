#include "wal.hpp"

namespace basonlite::wal {

void wal_truncate_before(const std::string& dir, std::uint64_t offset) {
    (void)dir; (void)offset;
}

} // namespace basonlite::wal