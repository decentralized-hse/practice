#include "wal/iterator.hpp"

#include "codec/record.hpp"

#include <utility>

namespace bason_db {

    WalIterator::WalIterator(std::vector<Entry> entries, size_t start_idx)
        : entries_{std::move(entries)}
        , it_{(start_idx < entries_.size() ? entries_.begin() + start_idx : entries_.end())} {
    }

    bool WalIterator::valid() const {
        return it_ != entries_.end();
    }

    void WalIterator::next() {
        if (valid()) {
            ++it_;
        }
    }

    uint64_t WalIterator::offset() const {
        return it_->offset;
    }

    const BasonRecord& WalIterator::record() const {
        return it_->record;
    }

} // namespace bason_db
