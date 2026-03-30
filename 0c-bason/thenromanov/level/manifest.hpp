#pragma once

#include "level/version_edit.hpp"

#include "sst/metadata.hpp"

#include "wal/writer.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace bason_db {

    class Manifest {
    public:
        Manifest() = default;

        Manifest(const Manifest&) = delete;
        Manifest(Manifest&& other) noexcept;

        Manifest& operator=(const Manifest&) = delete;
        Manifest& operator=(Manifest&& other) noexcept;

        ~Manifest() = default;

        static Manifest open(const std::filesystem::path& dir);

        void log_edit(const VersionEdit& edit);

        std::vector<VersionEdit> replay();

        void write_snapshot(const std::vector<SstMetadata>& files);

        const std::filesystem::path& dir() const;

    private:
        std::filesystem::path manifest_dir() const;

        std::filesystem::path dir_;
        std::unique_ptr<WalWriter> wal_writer_;
        mutable std::mutex mutex_;
    };

} // namespace bason_db
