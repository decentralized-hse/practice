#include "level/snapshot.hpp"

#include <stdexcept>

namespace bason_db {

    Snapshot::Snapshot(uint64_t offset)
        : offset{offset}
        , prev_{nullptr}
        , next_{nullptr} {
    }

    SnapshotList::SnapshotList() {
        head_.prev_ = &head_;
        head_.next_ = &head_;
    }

    const Snapshot* SnapshotList::front() const {
        if (empty()) {
            return nullptr;
        }
        return head_.next_;
    }

    const Snapshot* SnapshotList::back() const {
        if (empty()) {
            return nullptr;
        }
        return head_.prev_;
    }

    Snapshot* SnapshotList::emplace_back(uint64_t offset) {
        if (!empty() && back()->offset > offset) {
            throw std::runtime_error{"Offset must be greater than the last offset"};
        }

        auto* snapshot = new Snapshot{offset};
        snapshot->next_ = &head_;
        snapshot->prev_ = head_.prev_;
        snapshot->prev_->next_ = snapshot;
        snapshot->next_->prev_ = snapshot;
        return snapshot;
    }

    void SnapshotList::erase(Snapshot* snapshot) {
        snapshot->prev_->next_ = snapshot->next_;
        snapshot->next_->prev_ = snapshot->prev_;
        delete snapshot;
    }

    bool SnapshotList::empty() const {
        return head_.next_ == &head_;
    }

} // namespace bason_db
