#pragma once

#include "codec/record.hpp"

#include <cstdint>
#include <string>

namespace bason_db {

    struct DataRecord {
        std::string key;
        BasonRecord record;
        uint64_t offset = 0;
    };

    BasonRecord to_bason(const DataRecord& data_record);

    void from_bason(const BasonRecord& record, DataRecord& out);

    struct IndexRecord {
        std::string first_key;
        uint64_t block_offset = 0;
    };

    BasonRecord to_bason(const IndexRecord& record);

    void from_bason(const BasonRecord& record, IndexRecord& out);

} // namespace bason_db
