#include "level/compaction.hpp"

#include "memtable/merge_iterator.hpp"

#include "sst/iterator.hpp"
#include "sst/reader.hpp"
#include "sst/util.hpp"
#include "sst/writer.hpp"

#include "util/util.hpp"

#include <filesystem>
#include <iomanip>
#include <memory>
#include <numeric>
#include <sstream>

namespace bason_db {

    CompactionResult run_compaction(int source_level, const std::vector<FileInfo>& input_files,
                                    const std::vector<FileInfo>& target_files,
                                    const std::filesystem::path& sst_dir,
                                    std::atomic_uint64_t& next_file_number,
                                    uint64_t min_live_offset, const CompactionOptions& opts) {
        auto result = CompactionResult{};
        int target_level = source_level + 1;

        auto iters = std::vector<std::unique_ptr<SortedIterator>>{};
        for (const auto& file_info : input_files) {
            auto it = file_info.reader->scan();
            if (it.valid()) {
                iters.emplace_back(std::make_unique<SstIterator>(std::move(it)));
            }
            result.bytes_read += file_info.meta.file_size;
        }
        for (const auto& file_info : target_files) {
            auto it = file_info.reader->scan();
            if (it.valid()) {
                iters.emplace_back(std::make_unique<SstIterator>(std::move(it)));
            }
            result.bytes_read += file_info.meta.file_size;
        }

        auto merger = MergeIterator{std::move(iters), min_live_offset,
                                    std::numeric_limits<uint64_t>::max(), false};

        std::filesystem::create_directories(sst_dir);

        auto writer = std::unique_ptr<SstWriter>();
        size_t current_bytes = 0;

        auto finish_writer = [&]() {
            if (writer != nullptr) {
                auto meta = writer->finish();
                meta.level = target_level;
                result.edit.additions.emplace_back(VersionEdit::FileAdd{
                    .level = target_level,
                    .file = meta.path,
                    .file_size = meta.file_size,
                    .first_key = meta.first_key,
                    .last_key = meta.last_key,
                    .min_offset = meta.min_offset,
                    .max_offset = meta.max_offset,
                });
                result.bytes_written += meta.file_size;
                writer.reset();
            }
        };

        while (merger.valid()) {
            if (writer == nullptr) {
                auto path = make_sst_path(sst_dir, next_file_number++);
                writer = std::make_unique<SstWriter>(
                    SstWriter::open(path, SstWriter::Options{
                                              .block_size = opts.block_size,
                                              .bloom_bits_per_key = opts.bloom_bits_per_key,
                                          }));
                current_bytes = 0;
            }

            writer->add(merger.key(), merger.record(), merger.offset());

            current_bytes +=
                merger.key().size() + record_size(merger.record()) + sizeof(merger.offset());

            merger.next();

            if (current_bytes >= opts.max_sst_size) {
                finish_writer();
            }
        }
        finish_writer();

        for (const auto& file_info : input_files) {
            result.edit.removals.emplace_back(VersionEdit::FileRemove{
                .level = source_level,
                .file = file_info.meta.path,
            });
        }

        for (const auto& file_info : target_files) {
            result.edit.removals.emplace_back(VersionEdit::FileRemove{
                .level = target_level,
                .file = file_info.meta.path,
            });
        }

        return result;
    }

} // namespace bason_db
