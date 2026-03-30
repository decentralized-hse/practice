#include "level/manifest.hpp"

#include "codec/record.hpp"

#include "level/constants.hpp"

#include "wal/reader.hpp"

#include <cstring>
#include <string_view>

namespace bason_db {

    namespace {

        constexpr auto kManifest = std::string_view{"manifest"};

    } // namespace

    Manifest::Manifest(Manifest&& other) noexcept
        : dir_{std::move(other.dir_)}
        , wal_writer_{std::move(other.wal_writer_)} {
    }

    Manifest& Manifest::operator=(Manifest&& other) noexcept {
        if (this != &other) {
            dir_ = std::move(other.dir_);
            wal_writer_ = std::move(other.wal_writer_);
        }
        return *this;
    }

    Manifest Manifest::open(const std::filesystem::path& dir) {
        std::filesystem::create_directories(dir);
        auto manifest = Manifest{};
        manifest.dir_ = dir;

        auto mdir = manifest.manifest_dir();
        std::filesystem::create_directories(mdir);

        manifest.wal_writer_ = std::make_unique<WalWriter>(WalWriter::open(mdir));
        return manifest;
    }

    void Manifest::log_edit(const VersionEdit& edit) {
        auto lock = std::unique_lock{mutex_};

        auto bason_record = to_bason(edit);
        wal_writer_->append(bason_record);
        wal_writer_->checkpoint();
        wal_writer_->sync();
    }

    std::vector<VersionEdit> Manifest::replay() {
        auto edits = std::vector<VersionEdit>{};
        auto mdir = manifest_dir();

        if (!std::filesystem::exists(mdir)) {
            return edits;
        }

        auto reader = WalReader::open(mdir);
        reader.recover();

        auto it = reader.scan(0);
        while (it.valid()) {
            try {
                from_bason(it.record(), edits.emplace_back());
            } catch (...) {
                // Skip corrupted entries
            }
            it.next();
        }

        return edits;
    }

    void Manifest::write_snapshot(const std::vector<SstMetadata>& files) {
        auto lock = std::unique_lock{mutex_};

        auto mdir = manifest_dir();
        if (std::filesystem::exists(mdir)) {
            std::filesystem::remove_all(mdir);
        }
        std::filesystem::create_directories(mdir);

        wal_writer_ = std::make_unique<WalWriter>(WalWriter::open(mdir));

        auto edit = VersionEdit{};
        for (const auto& meta : files) {
            edit.additions.emplace_back(VersionEdit::FileAdd{
                .level = meta.level,
                .file = meta.path,
                .file_size = meta.file_size,
                .first_key = meta.first_key,
                .last_key = meta.last_key,
                .min_offset = meta.min_offset,
                .max_offset = meta.max_offset,
            });
        }

        auto bason_record = to_bason(edit);
        wal_writer_->append(bason_record);
        wal_writer_->checkpoint();
        wal_writer_->sync();
    }

    const std::filesystem::path& Manifest::dir() const {
        return dir_;
    }

    std::filesystem::path Manifest::manifest_dir() const {
        return dir_ / kManifest;
    }

} // namespace bason_db
