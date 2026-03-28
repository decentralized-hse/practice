#pragma once

#include <cstdint>

namespace bason_db {

    class Snapshot {
    public:
        explicit Snapshot(uint64_t offset = 0);

        const uint64_t offset = 0;

    private:
        friend class SnapshotList;

        Snapshot* prev_ = nullptr;
        Snapshot* next_ = nullptr;
    };

    class SnapshotList {
    public:
        SnapshotList();

        const Snapshot* front() const;

        const Snapshot* back() const;

        Snapshot* emplace_back(uint64_t offset);

        void erase(Snapshot* snapshot);

        bool empty() const;

    private:
        Snapshot head_;
    };

} // namespace bason_db
