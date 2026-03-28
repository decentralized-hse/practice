#pragma once

#include "codec/record.hpp"

#include <string>

namespace bason_db {

    struct WalRecord {
        std::string key;
        BasonRecord record;
    };

    BasonRecord to_bason(const WalRecord& wal_record);

    void from_bason(const BasonRecord& record, WalRecord& out);

} // namespace bason_db
