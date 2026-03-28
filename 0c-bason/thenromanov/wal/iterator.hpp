#pragma once

#include "codec/record.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bason_db {

    class WalIterator {
    public:
        struct Entry {
            uint64_t offset;
            BasonRecord record;
        };

        explicit WalIterator(std::vector<Entry> entries, size_t start_idx = 0);

        bool valid() const;

        void next();

        uint64_t offset() const; // global byte offset of current record

        const BasonRecord& record() const;

    private:
        std::vector<Entry> entries_;
        std::vector<Entry>::const_iterator it_;
    };

} // namespace bason_db
