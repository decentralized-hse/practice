#pragma once

#include "codec/record.hpp"

#include <string>

namespace bason_db {

    class SortedIterator {
    public:
        virtual ~SortedIterator() = default;
        virtual bool valid() const = 0;
        virtual void next() = 0;
        virtual const std::string& key() const = 0;
        virtual const BasonRecord& record() const = 0;
        virtual uint64_t offset() const = 0;
    };

} // namespace bason_db
