#pragma once

#include "codec/record.hpp"

#include "sst/iterator.hpp"
#include "sst/metadata.hpp"
#include "sst/record.hpp"

#include <filesystem>
#include <numeric>
#include <string>
#include <vector>

namespace bason_db {

    class SstWriter {
    public:
        struct Options {
            size_t block_size = 4096;
            int bloom_bits_per_key = 10;
            // Compression type (none, snappy, zstd)
        };

        static SstWriter open(const std::filesystem::path& path);

        static SstWriter open(const std::filesystem::path& path, Options opts);

        // Add a record. Keys must be added in strictly ascending sorted
        // order. Throws if order is violated.
        void add(const std::string& key, const BasonRecord& record, uint64_t offset = 0);

        // Finalize: write filter block, index block, footer. Flush and
        // close the file. Returns metadata (file size, record count,
        // first key, last key).
        SstMetadata finish();

    private:
        std::filesystem::path path_;
        Options opts_;

        std::vector<BasonRecord> records_;
        std::string last_internal_key_;
        std::string last_user_key_;
        std::string first_user_key_;
        uint64_t min_offset_ = std::numeric_limits<uint64_t>::max();
        uint64_t max_offset_ = std::numeric_limits<uint64_t>::min();
    };

} // namespace bason_db
