#include "memtable/memtable.hpp"

#include "memtable/sorted_iterator.hpp"

#include "util/util.hpp"

namespace bason_db {

    namespace {

        class MemtableIterator final: public SortedIterator {
        public:
            using MapIterator = Memtable::MapType::const_iterator;

            explicit MemtableIterator(MapIterator begin, MapIterator end)
                : it_(begin)
                , end_(end) {
            }

            bool valid() const override {
                return it_ != end_;
            }

            void next() override {
                if (valid()) {
                    ++it_;
                }
            }

            const std::string& key() const override {
                return it_->first.key;
            }

            const BasonRecord& record() const override {
                return it_->second;
            }

            uint64_t offset() const override {
                return it_->first.offset;
            }

        private:
            MapIterator it_;
            MapIterator end_;
        };

        size_t entry_size(const std::string& key, const BasonRecord& record) {
            return key.size() + record_size(record) + sizeof(Memtable::Key) + 64;
        }

    } // namespace

    bool Memtable::Key::operator<(const Key& other) const {
        if (key != other.key) {
            return key < other.key;
        }
        return offset > other.offset;
    }

    void Memtable::put(const std::string& key, const BasonRecord& record, uint64_t offset) {
        data_.emplace(
            Key{
                .key = key,
                .offset = offset,
            },
            record);
        memory_usage_ += entry_size(key, record);
    }

    std::optional<std::pair<BasonRecord, uint64_t>> Memtable::get(const std::string& key,
                                                                  uint64_t max_offset) const {
        auto it = data_.lower_bound(Key{
            .key = key,
            .offset = max_offset,
        });
        if (it != data_.end() && it->first.key == key) {
            return std::make_pair(it->second, it->first.offset);
        }
        return std::nullopt;
    }

    std::unique_ptr<SortedIterator> Memtable::scan(const std::string& start,
                                                   const std::string& end) const {
        auto begin_it = start.empty() ? data_.begin()
                                      : data_.lower_bound(Key{
                                            .key = start,
                                            .offset = std::numeric_limits<uint64_t>::max(),
                                        });
        auto end_it = end.empty() ? data_.end()
                                  : data_.lower_bound(Key{
                                        .key = end,
                                        .offset = std::numeric_limits<uint64_t>::max(),
                                    });
        return std::make_unique<MemtableIterator>(begin_it, end_it);
    }

    size_t Memtable::memory_usage() const {
        return memory_usage_;
    }

    size_t Memtable::count() const {
        return data_.size();
    }

    std::shared_ptr<const Memtable> Memtable::freeze() {
        auto frozen = std::make_shared<Memtable>();
        frozen->data_ = std::move(data_);
        frozen->memory_usage_ = memory_usage_;
        data_.clear();
        memory_usage_ = 0;
        return frozen;
    }

} // namespace bason_db
