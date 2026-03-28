#include "memtable/merge_iterator.hpp"

#include "util/util.hpp"

#include <algorithm>

namespace bason_db {

    MergeIterator::MergeIterator(std::vector<std::unique_ptr<SortedIterator>> sources,
                                 uint64_t min_live_offset, uint64_t max_visible_offset,
                                 bool deduplicate)
        : sources_{std::move(sources)}
        , min_live_offset_{min_live_offset}
        , max_visible_offset_{max_visible_offset}
        , deduplicate_{deduplicate} {
        for (size_t i = 0; i < sources_.size(); ++i) {
            if (sources_[i]->valid()) {
                heap_.emplace_back(HeapEntry{
                    .key = sources_[i]->key(),
                    .record = sources_[i]->record(),
                    .offset = sources_[i]->offset(),
                    .source_idx = i,
                });
            }
        }

        std::make_heap(heap_.begin(), heap_.end(), std::greater<HeapEntry>{});
        advance();
    }

    bool MergeIterator::valid() const {
        return is_valid_;
    }

    void MergeIterator::next() {
        advance();
    }

    const std::string& MergeIterator::key() const {
        return current_.key;
    }

    const BasonRecord& MergeIterator::record() const {
        return current_.record;
    }

    uint64_t MergeIterator::offset() const {
        return current_.offset;
    }

    void MergeIterator::advance() {
        is_valid_ = false;

        while (!heap_.empty()) {
            std::pop_heap(heap_.begin(), heap_.end(), std::greater<HeapEntry>{});
            auto top = std::move(heap_.back());
            heap_.pop_back();

            sources_[top.source_idx]->next();
            if (sources_[top.source_idx]->valid()) {
                heap_.emplace_back(HeapEntry{
                    .key = sources_[top.source_idx]->key(),
                    .record = sources_[top.source_idx]->record(),
                    .offset = sources_[top.source_idx]->offset(),
                    .source_idx = top.source_idx,
                });
                std::push_heap(heap_.begin(), heap_.end(), std::greater<HeapEntry>{});
            }

            if (!deduplicate_) {
                if (top.offset < min_live_offset_) {
                    while (!heap_.empty()) {
                        const HeapEntry& next_top = heap_.front();
                        if (next_top.key != top.key) {
                            break;
                        }

                        std::pop_heap(heap_.begin(), heap_.end(), std::greater<HeapEntry>{});
                        auto dup = std::move(heap_.back());
                        heap_.pop_back();

                        sources_[dup.source_idx]->next();
                        if (sources_[dup.source_idx]->valid()) {
                            heap_.emplace_back(HeapEntry{
                                .key = sources_[dup.source_idx]->key(),
                                .record = sources_[dup.source_idx]->record(),
                                .offset = sources_[dup.source_idx]->offset(),
                                .source_idx = dup.source_idx,
                            });
                            std::push_heap(heap_.begin(), heap_.end(), std::greater<HeapEntry>{});
                        }
                    }

                    if (is_tombstone(top.record)) {
                        continue;
                    }
                }

                current_ = std::move(top);
                is_valid_ = true;
                return;
            }

            auto is_visible = (top.offset <= max_visible_offset_);

            while (!heap_.empty()) {
                const HeapEntry& next_top = heap_.front();
                if (next_top.key != top.key) {
                    break;
                }

                std::pop_heap(heap_.begin(), heap_.end(), std::greater<HeapEntry>{});
                auto dup = std::move(heap_.back());
                heap_.pop_back();

                if (dup.offset > top.offset) {
                    top.record = dup.record;
                    top.offset = dup.offset;
                }

                if (dup.offset <= max_visible_offset_) {
                    if (!is_visible || dup.offset > top.offset) {
                        top.record = dup.record;
                        top.offset = dup.offset;
                        is_visible = true;
                    }
                }

                sources_[dup.source_idx]->next();
                if (sources_[dup.source_idx]->valid()) {
                    heap_.emplace_back(HeapEntry{
                        .key = sources_[dup.source_idx]->key(),
                        .record = sources_[dup.source_idx]->record(),
                        .offset = sources_[dup.source_idx]->offset(),
                        .source_idx = dup.source_idx,
                    });
                    std::push_heap(heap_.begin(), heap_.end(), std::greater<HeapEntry>{});
                }
            }

            if (!is_visible) {
                continue;
            }

            if (is_tombstone(top.record) && top.offset < min_live_offset_) {
                continue;
            }

            current_ = std::move(top);
            is_valid_ = true;
            return;
        }
    }

    bool MergeIterator::HeapEntry::operator>(const HeapEntry& other) const {
        if (key != other.key) {
            return key > other.key;
        }
        return offset < other.offset;
    }

} // namespace bason_db
