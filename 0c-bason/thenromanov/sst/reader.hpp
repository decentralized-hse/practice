#pragma once

#include "codec/record.hpp"

#include "sst/iterator.hpp"
#include "sst/metadata.hpp"
#include "sst/record.hpp"

#include <filesystem>
#include <optional>

namespace bason_db {

    class SstReader {
    public:
        static SstReader open(const std::filesystem::path& path);

        // Point lookup. Returns nullopt if key is not found. Uses bloom
        // filter to short-circuit when possible.
        std::optional<std::pair<BasonRecord, uint64_t>>
        get(const std::string& key, uint64_t max_offset = std::numeric_limits<uint64_t>::max());

        // Range scan from start (inclusive) to end (exclusive).
        // Empty start means beginning of file. Empty end means end of
        // file.
        SstIterator scan(const std::string& start = "", const std::string& end = "");

        // Check bloom filter without reading data. Returns false if the
        // key is definitely not present. Returns true if the key might
        // be present.
        bool may_contain(const std::string& key);

        // File metadata.
        SstMetadata metadata() const;

    private:
        std::filesystem::path path_;
        SstMetadata meta_;
        std::vector<DataRecord> records_;
    };

} // namespace bason_db
