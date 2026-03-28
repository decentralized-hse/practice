#pragma once

#include "codec/record.hpp"

#include "level/constants.hpp"
#include "level/level_metrics.hpp"
#include "level/manifest.hpp"
#include "level/snapshot.hpp"
#include "level/version.hpp"

#include "memtable/memtable.hpp"
#include "memtable/sorted_iterator.hpp"

#include "wal/writer.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>

namespace bason_db {

    struct ReadOptions {
        const Snapshot* snapshot = nullptr;
    };

    class BasonLevel {
    public:
        struct Options {
            std::filesystem::path dir;
            size_t memtable_size = 4 * 1024 * 1024; // 4 MB
            size_t l0_compaction_trigger = 4;
            size_t level_size_base = 64 * 1024 * 1024; // 64 MB
            size_t level_size_ratio = 10;
            size_t max_levels = kTotalLevels;
            size_t block_size = 4096;
            int bloom_bits_per_key = 10;
        };

        static std::unique_ptr<BasonLevel> open(Options opts);

        BasonLevel(const BasonLevel&) = delete;
        BasonLevel& operator=(const BasonLevel&) = delete;

        ~BasonLevel();

        void put(const std::string& key, const BasonRecord& value);

        std::optional<BasonRecord> get(const std::string& key, const ReadOptions& opts = {});

        void del(const std::string& key);

        // Range scan.
        std::unique_ptr<SortedIterator> scan(const std::string& start = "",
                                             const std::string& end = "",
                                             const ReadOptions& opts = {});

        // Create a snapshot. Reads through this snapshot see a consistent
        // view at the time of creation. The snapshot pins tombstones
        // above its offset from being garbage collected.
        std::shared_ptr<Snapshot> snapshot();

        // Force compaction of a level (for testing).
        void compact_level(int level);

        void close();

        LevelMetrics metrics() const;

    private:
        BasonLevel() = default;

        std::filesystem::path sst_dir() const;

        std::filesystem::path wal_dir() const;

        void recover();

        void make_room_for_write(std::unique_lock<std::mutex>& lock);

        void flush_memtable(const std::shared_ptr<const Memtable>& memtables);

        uint64_t min_snapshot_offset() const;

        void run_background();

        Options opts_;

        mutable std::mutex mutex_;

        std::unique_ptr<Memtable> active_memtable_;
        std::shared_ptr<const Memtable> frozen_memtable_;

        std::unique_ptr<WalWriter> wal_writer_;
        Manifest manifest_;
        Version version_;

        std::atomic_uint64_t next_file_number_ = 1;
        uint64_t last_flush_offset_ = 0;

        LevelMetrics metrics_{};

        std::jthread bg_thread_;
        std::condition_variable bg_cv_;
        std::condition_variable flush_done_cv_;
        std::atomic_bool shutdown_ = false;
        std::atomic_bool closed_ = false;
        bool flush_pending_ = false;
        bool compact_pending_ = false;

        mutable std::mutex snapshot_list_mutex_;

        SnapshotList snapshot_list_;

        mutable std::mutex compaction_mutex_;
    };

} // namespace bason_db
