#pragma once

#include "codec/record.hpp"

#include "memtable/sorted_iterator.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace bason_db {

    class Memtable {
    public:
        struct Key {
            std::string key;
            uint64_t offset = 0;

            bool operator<(const Key& other) const;
        };

        using MapType = std::map<Key, BasonRecord>;

        // Insert or update a record. The offset is the WAL offset (LSN)
        // of the record, used for conflict resolution.
        void put(const std::string& key, const BasonRecord& record, uint64_t offset);

        // Point lookup. Returns the record and its offset, or nullopt.
        std::optional<std::pair<BasonRecord, uint64_t>>
        get(const std::string& key,
            uint64_t max_offset = std::numeric_limits<uint64_t>::max()) const;

        // Range scan.
        std::unique_ptr<SortedIterator> scan(const std::string& start = "",
                                             const std::string& end = "") const;

        // Total memory usage in bytes (keys + values + skip list
        // overhead).
        size_t memory_usage() const;

        // Number of entries.
        size_t count() const;

        // Freeze: return an immutable snapshot of this memtable and
        // reset the mutable state. The frozen memtable supports reads
        // but not writes.
        std::shared_ptr<const Memtable> freeze();

    private:
        MapType data_;
        size_t memory_usage_ = 0;
    };

} // namespace bason_db
