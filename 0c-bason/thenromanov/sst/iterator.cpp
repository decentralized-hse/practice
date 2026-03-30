#include "sst/iterator.hpp"

#include <algorithm>
#include <optional>

namespace bason_db {

    SstIterator::SstIterator(const std::vector<DataRecord>& records, const std::string& start,
                             const std::string& end)
        : it_{records.begin()}
        , end_it_{records.end()} {
        if (!start.empty()) {
            it_ = std::lower_bound(
                records.begin(), records.end(), start,
                [](const DataRecord& entry, const std::string& str) { return entry.key < str; });
        }
        if (!end.empty()) {
            end_it_ = std::lower_bound(
                records.begin(), records.end(), end,
                [](const DataRecord& entry, const std::string& str) { return entry.key < str; });
        }
        if (it_ != records.end() && !end.empty() && it_->key >= end) {
            it_ = end_it_;
        }
    }

    bool SstIterator::valid() const {
        if (it_ == end_it_) {
            return false;
        }
        return true;
    }

    void SstIterator::next() {
        if (valid()) {
            ++it_;
        }
    }

    const std::string& SstIterator::key() const {
        return it_->key;
    }

    const BasonRecord& SstIterator::record() const {
        return it_->record;
    }

    uint64_t SstIterator::offset() const {
        return it_->offset;
    }

} // namespace bason_db
