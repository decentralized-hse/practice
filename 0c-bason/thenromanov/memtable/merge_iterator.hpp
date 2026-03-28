#pragma once

#include "codec/record.hpp"

#include "memtable/sorted_iterator.hpp"

#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace bason_db {

    class MergeIterator final: public SortedIterator {
    public:
        // Construct from a list of sorted iterators. Each iterator
        // must yield (key, record, offset) triples in ascending key
        // order. When multiple iterators yield the same key, the one
        // with the highest offset wins. Tombstones with offsets below
        // min_live_offset are suppressed.
        explicit MergeIterator(std::vector<std::unique_ptr<SortedIterator>> sources,
                               uint64_t min_live_offset = 0,
                               uint64_t max_visible_offset = std::numeric_limits<uint64_t>::max(),
                               bool deduplicate = true);

        ~MergeIterator() final = default;

        bool valid() const final;
        void next() final;
        const std::string& key() const final;
        const BasonRecord& record() const final;
        uint64_t offset() const final;

    private:
        void advance();

        struct HeapEntry {
            std::string key;
            BasonRecord record;
            uint64_t offset = 0;
            size_t source_idx = 0;

            bool operator>(const HeapEntry& other) const;
        };

        std::vector<std::unique_ptr<SortedIterator>> sources_;
        uint64_t min_live_offset_ = 0;
        uint64_t max_visible_offset_ = std::numeric_limits<uint64_t>::max();
        bool deduplicate_ = true;

        HeapEntry current_;
        bool is_valid_ = false;

        std::vector<HeapEntry> heap_;
    };

} // namespace bason_db
