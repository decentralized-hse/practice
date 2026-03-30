#pragma once

#include "codec/record.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace bason_db {

    bool is_tombstone(const BasonRecord& record);

    BasonRecord make_tombstone(const std::string& key = "");

    BasonRecord make_number_record(const std::string& key, uint64_t value);

    BasonRecord make_string_record(const std::string& key, const std::string& value);

    BasonRecord make_blob_record(const std::string& key, const BasonRecord& payload);

    void extract_blob_record(const BasonRecord& child, BasonRecord& out);

    size_t record_size(const BasonRecord& record);

    std::filesystem::path get_test_tmp_dir();

} // namespace bason_db
