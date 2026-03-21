#include "persistence_manager.hpp"
#include <sys/stat.h>
#include <sys/types.h>

namespace bason {

PersistenceManager::PersistenceManager(const std::string& data_dir)
    : data_dir_(data_dir), aof_enabled_(true) {
    mkdir(data_dir_.c_str(), 0755);
    aof_file_.open(get_aof_path(), std::ios::binary | std::ios::app);
}

PersistenceManager::~PersistenceManager() {
    if (aof_file_.is_open()) {
        aof_file_.close();
    }
}

std::string PersistenceManager::get_snapshot_path() const {
    return data_dir_ + "/dump.bason";
}

std::string PersistenceManager::get_aof_path() const {
    return data_dir_ + "/appendonly.aof";
}

void PersistenceManager::log_command(const BasonRecord& command) {
    if (!aof_enabled_ || !aof_file_.is_open()) return;
    auto encoded = BasonCodec::encode(command);
    aof_file_.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
}

void PersistenceManager::sync() {
    if (aof_file_.is_open()) {
        aof_file_.flush();
    }
}

void PersistenceManager::snapshot(const BasonRedisStore& store, const std::string& path) {
    BasonRecord root(BasonType::Object);
    auto encoded = BasonCodec::encode(root);

    std::ofstream file(path.empty() ? get_snapshot_path() : path,
                       std::ios::binary | std::ios::trunc);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
        file.close();
    }
}

void PersistenceManager::recover(BasonRedisStore& store) {
    std::string snapshot_path = get_snapshot_path();
    std::ifstream snapshot_file(snapshot_path, std::ios::binary);

    if (snapshot_file.is_open()) {
        snapshot_file.seekg(0, std::ios::end);
        size_t size = snapshot_file.tellg();
        snapshot_file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        snapshot_file.read(reinterpret_cast<char*>(data.data()), size);
        snapshot_file.close();

        if (size > 0) {
            try {
                BasonCodec::decode_all(data.data(), size);
            } catch (...) {}
        }
    }

    std::string aof_path = get_aof_path();
    std::ifstream aof_file(aof_path, std::ios::binary);

    if (aof_file.is_open()) {
        aof_file.seekg(0, std::ios::end);
        size_t size = aof_file.tellg();
        aof_file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        aof_file.read(reinterpret_cast<char*>(data.data()), size);
        aof_file.close();

        if (size > 0) {
            try {
                BasonCodec::decode_all(data.data(), size);
            } catch (...) {}
        }
    }
}

}
