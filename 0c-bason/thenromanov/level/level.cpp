#include "level/level.hpp"

#include "level/compaction.hpp"

#include "memtable/merge_iterator.hpp"

#include "sst/iterator.hpp"
#include "sst/reader.hpp"
#include "sst/util.hpp"
#include "sst/writer.hpp"

#include "wal/constants.hpp"
#include "wal/reader.hpp"
#include "wal/record.hpp"

#include "util/util.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace bason_db {

    std::unique_ptr<BasonLevel> BasonLevel::open(Options opts) {
        auto db = std::unique_ptr<BasonLevel>(new BasonLevel());
        db->opts_ = std::move(opts);
        std::filesystem::create_directories(db->opts_.dir);
        std::filesystem::create_directories(db->sst_dir());
        std::filesystem::create_directories(db->wal_dir());

        db->active_memtable_ = std::make_unique<Memtable>();
        db->manifest_ = Manifest::open(db->opts_.dir);

        db->recover();

        auto* raw = db.get();
        db->bg_thread_ = std::jthread([raw]() { raw->run_background(); });

        return db;
    }

    BasonLevel::~BasonLevel() {
        if (!closed_.load()) {
            shutdown_.store(true);
            bg_cv_.notify_one();
            flush_done_cv_.notify_all();
            if (bg_thread_.joinable()) {
                bg_thread_.join();
            }
        }
    }

    void BasonLevel::put(const std::string& key, const BasonRecord& value) {
        if (closed_.load()) {
            throw std::runtime_error{"Database is closed"};
        }

        auto lock = std::unique_lock{mutex_};

        auto offset = wal_writer_->append(to_bason(WalRecord{
            .key = key,
            .record = value,
        }));
        wal_writer_->checkpoint();
        wal_writer_->sync();

        active_memtable_->put(key, value, offset);
        metrics_.user_bytes_written += key.size() + record_size(value);

        if (active_memtable_->memory_usage() >= opts_.memtable_size) {
            make_room_for_write(lock);
        }
    }

    std::optional<BasonRecord> BasonLevel::get(const std::string& key, const ReadOptions& opts) {
        if (closed_.load()) {
            throw std::runtime_error{"Database is closed"};
        }

        auto max_offset =
            opts.snapshot != nullptr ? opts.snapshot->offset : std::numeric_limits<uint64_t>::max();

        auto l0_files = std::vector<FileInfo>{};
        auto level_files = std::vector<std::vector<FileInfo>>{};

        {
            auto lock = std::scoped_lock{mutex_};

            auto result = active_memtable_->get(key, max_offset);
            if (result.has_value()) {
                if (is_tombstone(result->first)) {
                    return std::nullopt;
                }
                return result->first;
            }

            if (frozen_memtable_ != nullptr) {
                result = frozen_memtable_->get(key, max_offset);
                if (result.has_value()) {
                    if (is_tombstone(result->first)) {
                        return std::nullopt;
                    }
                    return result->first;
                }
            }

            l0_files = version_.sorted_files(0);
            level_files.reserve(opts_.max_levels);
            for (int level = 1; level < static_cast<int>(opts_.max_levels); ++level) {
                level_files.emplace_back(version_.sorted_files(level));
            }
        }

        std::sort(l0_files.begin(), l0_files.end(), [](const FileInfo& lhs, const FileInfo& rhs) {
            return lhs.meta.max_offset > rhs.meta.max_offset;
        });

        for (const auto& file_info : l0_files) {
            if (file_info.meta.min_offset > max_offset) {
                continue;
            }
            if (!file_info.reader->may_contain(key)) {
                continue;
            }
            auto result = file_info.reader->get(key, max_offset);
            if (result.has_value()) {
                if (is_tombstone(result->first)) {
                    return std::nullopt;
                }
                return result->first;
            }
        }

        for (const auto& file_infos : level_files) {
            if (file_infos.empty()) {
                continue;
            }

            auto it = std::upper_bound(file_infos.begin(), file_infos.end(), key,
                                       [](const std::string& key, const FileInfo& file_info) {
                                           return key < file_info.meta.first_key;
                                       });
            if (it != file_infos.begin()) {
                --it;
                if (key >= it->meta.first_key && key <= it->meta.last_key) {
                    if (!it->reader->may_contain(key)) {
                        continue;
                    }
                    auto result = it->reader->get(key, max_offset);
                    if (result) {
                        if (is_tombstone(result->first)) {
                            return std::nullopt;
                        }
                        return result->first;
                    }
                }
            }
        }

        return std::nullopt;
    }

    void BasonLevel::del(const std::string& key) {
        put(key, make_tombstone(key));
    }

    std::unique_ptr<SortedIterator>
    BasonLevel::scan(const std::string& start, const std::string& end, const ReadOptions& opts) {
        if (closed_.load()) {
            throw std::runtime_error{"Database is closed"};
        }

        auto max_offset =
            opts.snapshot != nullptr ? opts.snapshot->offset : std::numeric_limits<uint64_t>::max();

        auto lock = std::scoped_lock{mutex_};

        auto iters = std::vector<std::unique_ptr<SortedIterator>>{};
        iters.emplace_back(active_memtable_->scan(start, end));
        if (frozen_memtable_ != nullptr) {
            iters.emplace_back(frozen_memtable_->scan(start, end));
        }

        for (int level = 0; level < static_cast<int>(opts_.max_levels); ++level) {
            for (const auto& file_info : version_.sorted_files(level)) {
                auto it = file_info.reader->scan(start, end);
                if (it.valid()) {
                    iters.emplace_back(std::make_unique<SstIterator>(std::move(it)));
                }
            }
        }

        return std::make_unique<MergeIterator>(std::move(iters), 0, max_offset);
    }

    std::shared_ptr<Snapshot> BasonLevel::snapshot() {
        auto lock = std::scoped_lock{mutex_};

        auto offset = wal_writer_->current_offset() - 1;

        Snapshot* raw = nullptr;
        {
            auto snapshot_list_lock = std::scoped_lock{snapshot_list_mutex_};
            raw = snapshot_list_.emplace_back(offset);
        }

        return {raw, [this](Snapshot* snapshot) {
                    auto snapshot_list_lock = std::scoped_lock{snapshot_list_mutex_};
                    snapshot_list_.erase(snapshot);
                }};
    }

    void BasonLevel::compact_level(int level) {
        auto compaction_lock = std::scoped_lock{compaction_mutex_};

        auto inputs = std::vector<FileInfo>{};
        auto target_files = std::vector<FileInfo>{};

        uint64_t min_live_offset = 0;

        {
            auto lock = std::scoped_lock{mutex_};

            if (level < 0 || level >= static_cast<int>(opts_.max_levels) - 1) {
                return;
            }

            if (level == 0) {
                inputs.append_range(version_.sorted_files(0));
            } else {
                auto files = version_.sorted_files(level);
                if (files.empty()) {
                    return;
                }

                size_t best_overlap = 0;
                size_t best_overlap_file_idx = 0;
                for (size_t i = 0; i < files.size(); ++i) {
                    auto overlapping_files =
                        version_.overlapping_files(level + 1, files[i].meta.first_key,
                                                   files[i].meta.last_key + std::string(1, '\xFF'));
                    if (overlapping_files.size() >= best_overlap) {
                        best_overlap = overlapping_files.size();
                        best_overlap_file_idx = i;
                    }
                }

                inputs.emplace_back(std::move(files[best_overlap_file_idx]));
            }

            if (inputs.empty()) {
                return;
            }

            auto min_key = std::string{};
            auto max_key = std::string{};
            for (const auto& file_info : inputs) {
                if (min_key.empty() || file_info.meta.first_key < min_key) {
                    min_key = file_info.meta.first_key;
                }
                if (max_key.empty() || file_info.meta.last_key > max_key) {
                    max_key = file_info.meta.last_key;
                }
            }

            target_files =
                version_.overlapping_files(level + 1, min_key, max_key + std::string(1, '\xFF'));

            min_live_offset = min_snapshot_offset();
        }

        auto compaction_result = run_compaction(level, inputs, target_files, sst_dir(),
                                                next_file_number_, min_live_offset,
                                                CompactionOptions{
                                                    .block_size = opts_.block_size,
                                                    .bloom_bits_per_key = opts_.bloom_bits_per_key,
                                                });

        auto files_to_delete = std::vector<std::filesystem::path>{};

        {
            auto lock = std::scoped_lock{mutex_};

            for (const auto& removal : compaction_result.edit.removals) {
                version_.remove_file(removal.level, removal.file);
                if (std::filesystem::exists(removal.file)) {
                    files_to_delete.emplace_back(removal.file);
                }
            }

            for (const auto& addition : compaction_result.edit.additions) {
                version_.add_file(addition.level, SstMetadata{
                                                      .path = addition.file,
                                                      .file_size = addition.file_size,
                                                      .first_key = addition.first_key,
                                                      .last_key = addition.last_key,
                                                      .level = addition.level,
                                                      .min_offset = addition.min_offset,
                                                      .max_offset = addition.max_offset,
                                                  });
            }

            manifest_.log_edit(compaction_result.edit);
            ++metrics_.total_compactions;
            metrics_.total_bytes_written += compaction_result.bytes_written;
            metrics_.total_bytes_read += compaction_result.bytes_read;
        }

        for (const auto& path : files_to_delete) {
            auto ec = std::error_code{};
            std::filesystem::remove(path, ec);
        }
    }

    void BasonLevel::close() {
        if (closed_.exchange(true)) {
            return;
        }

        {
            auto lock = std::unique_lock{mutex_};
            flush_done_cv_.wait(lock, [this] { return frozen_memtable_ == nullptr; });
        }

        {
            auto lock = std::unique_lock{mutex_};
            if (active_memtable_ && active_memtable_->count() > 0) {
                frozen_memtable_ = active_memtable_->freeze();
                active_memtable_ = std::make_unique<Memtable>();
                flush_pending_ = true;
                bg_cv_.notify_one();

                flush_done_cv_.wait(lock, [this] { return frozen_memtable_ == nullptr; });
            }
        }

        shutdown_.store(true);
        bg_cv_.notify_one();
        if (bg_thread_.joinable()) {
            bg_thread_.join();
        }

        if (wal_writer_) {
            auto lock = std::unique_lock{mutex_};
            wal_writer_->sync();
        }
    }

    LevelMetrics BasonLevel::metrics() const {
        auto lock = std::scoped_lock{mutex_};
        auto metrics = metrics_;
        for (size_t i = 0; i < opts_.max_levels; ++i) {
            metrics.num_files_per_level[i] = version_.num_files(static_cast<int>(i));
            metrics.bytes_per_level[i] = version_.total_bytes(static_cast<int>(i));
        }
        if (metrics.user_bytes_written > 0) {
            metrics.write_amplification = static_cast<double>(metrics.total_bytes_written) /
                                          static_cast<double>(metrics.user_bytes_written);
        }
        return metrics;
    }

    std::filesystem::path BasonLevel::sst_dir() const {
        return opts_.dir / kSst;
    }

    std::filesystem::path BasonLevel::wal_dir() const {
        return opts_.dir / kWal;
    }

    void BasonLevel::recover() {
        auto edits = manifest_.replay();

        {
            for (const auto& edit : edits) {
                for (const auto& addition : edit.additions) {
                    if (std::filesystem::exists(addition.file)) {
                        version_.add_file(addition.level, SstMetadata{
                                                              .path = addition.file,
                                                              .file_size = addition.file_size,
                                                              .first_key = addition.first_key,
                                                              .last_key = addition.last_key,
                                                              .level = addition.level,
                                                              .min_offset = addition.min_offset,
                                                              .max_offset = addition.max_offset,
                                                          });

                        auto stem = std::filesystem::path(addition.file).stem().string();
                        try {
                            auto num = std::stoull(stem);
                            if (num >= next_file_number_) {
                                next_file_number_ = num + 1;
                            }
                        } catch (...) {
                        }
                    }
                }

                for (const auto& removal : edit.removals) {
                    version_.remove_file(removal.level, removal.file);
                }

                last_flush_offset_ = std::max(last_flush_offset_, edit.flush_offset);
            }
        }

        if (std::filesystem::exists(wal_dir())) {
            auto reader = WalReader::open(wal_dir());
            reader.recover();
            auto it = reader.scan(last_flush_offset_);
            while (it.valid()) {
                auto wal_record = WalRecord{};
                from_bason(it.record(), wal_record);
                active_memtable_->put(wal_record.key, wal_record.record, it.offset());
                it.next();
            }
        }

        wal_writer_ = std::make_unique<WalWriter>(WalWriter::open(wal_dir()));
    }

    void BasonLevel::make_room_for_write(std::unique_lock<std::mutex>& lock) {
        flush_done_cv_.wait(lock,
                            [this] { return frozen_memtable_ == nullptr || shutdown_.load(); });
        if (shutdown_.load()) {
            return;
        }

        frozen_memtable_ = active_memtable_->freeze();
        active_memtable_ = std::make_unique<Memtable>();
        flush_pending_ = true;
        bg_cv_.notify_one();
    }

    void BasonLevel::flush_memtable(const std::shared_ptr<const Memtable>& memtable) {
        auto path = std::filesystem::path{};

        {
            auto lock = std::scoped_lock{mutex_};

            path = make_sst_path(sst_dir(), next_file_number_++);
        }

        auto writer = SstWriter::open(path, SstWriter::Options{
                                                .block_size = opts_.block_size,
                                                .bloom_bits_per_key = opts_.bloom_bits_per_key,
                                            });
        auto it = memtable->scan();

        uint64_t max_offset = 0;
        while (it->valid()) {
            writer.add(it->key(), it->record(), it->offset());
            max_offset = std::max(max_offset, it->offset());
            it->next();
        }

        auto meta = writer.finish();
        meta.level = 0;

        {
            auto lock = std::scoped_lock{mutex_};

            version_.add_file(0, meta);

            auto edit = VersionEdit{};
            edit.additions.emplace_back(VersionEdit::FileAdd{
                .level = 0,
                .file = meta.path,
                .file_size = meta.file_size,
                .first_key = meta.first_key,
                .last_key = meta.last_key,
                .min_offset = meta.min_offset,
                .max_offset = meta.max_offset,
            });

            edit.flush_offset = max_offset;
            manifest_.log_edit(edit);

            last_flush_offset_ = max_offset;
            frozen_memtable_.reset();
            metrics_.total_bytes_written += meta.file_size;

            if (version_.num_files(0) >= opts_.l0_compaction_trigger) {
                compact_pending_ = true;
            }
        }

        flush_done_cv_.notify_all();
    }

    uint64_t BasonLevel::min_snapshot_offset() const {
        auto lock = std::scoped_lock{snapshot_list_mutex_};

        if (snapshot_list_.empty()) {
            return std::numeric_limits<uint64_t>::max();
        }

        return snapshot_list_.front()->offset;
    }

    void BasonLevel::run_background() {
        while (true) {
            auto to_flush = std::shared_ptr<const Memtable>{};
            auto do_compact = false;

            {
                auto lock = std::unique_lock{mutex_};
                bg_cv_.wait(lock, [this] {
                    return shutdown_.load() || flush_pending_ || compact_pending_;
                });

                if (shutdown_.load() && !flush_pending_ && !compact_pending_) {
                    return;
                }

                if (flush_pending_) {
                    to_flush = frozen_memtable_;
                    flush_pending_ = false;
                }

                if (compact_pending_) {
                    do_compact = true;
                    compact_pending_ = false;
                }
            }

            if (to_flush != nullptr) {
                flush_memtable(to_flush);
            }

            if (do_compact) {
                compact_level(0);

                auto threshold = opts_.level_size_base;
                for (int level = 1; level < static_cast<int>(opts_.max_levels) - 1; ++level) {
                    auto need = false;
                    {
                        auto lock = std::scoped_lock{mutex_};
                        need = version_.total_bytes(static_cast<size_t>(level)) > threshold;
                    }
                    if (need) {
                        compact_level(level);
                    }

                    threshold *= opts_.level_size_ratio;
                }

                {
                    auto lock = std::scoped_lock{mutex_};

                    if (version_.num_files(0) >= opts_.l0_compaction_trigger) {
                        compact_pending_ = true;
                        bg_cv_.notify_one();
                        continue;
                    }

                    auto threshold = opts_.level_size_base;
                    for (int level = 1; level < static_cast<int>(opts_.max_levels) - 1; ++level) {
                        if (version_.total_bytes(static_cast<size_t>(level)) > threshold) {
                            compact_pending_ = true;
                            bg_cv_.notify_one();
                            break;
                        }

                        threshold *= opts_.level_size_ratio;
                    }
                }
            }
        }
    }

} // namespace bason_db
