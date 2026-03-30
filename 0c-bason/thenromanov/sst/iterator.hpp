#pragma once

#include "codec/record.hpp"

#include "memtable/sorted_iterator.hpp"

#include "sst/record.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bason_db {

    class SstIterator final: public SortedIterator {
    public:
        explicit SstIterator(const std::vector<DataRecord>& records, const std::string& start = "",
                             const std::string& end = "");

        ~SstIterator() final = default;

        bool valid() const final;

        void next() final;

        const std::string& key() const final;

        const BasonRecord& record() const final;

        uint64_t offset() const final;

    private:
        std::vector<DataRecord>::const_iterator it_;
        std::vector<DataRecord>::const_iterator end_it_;
    };

} // namespace bason_db
