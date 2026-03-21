#pragma once

#include "bason_redis_store.hpp"
#include "../wal/wal.hpp"
#include <string>
#include <fstream>

namespace bason {

class PersistenceManager {
public:
    PersistenceManager(const std::string& data_dir);
    ~PersistenceManager();

    void log_command(const BasonRecord& command);
    void snapshot(const BasonRedisStore& store, const std::string& path);
    void recover(BasonRedisStore& store);
    void sync();

private:
    std::string data_dir_;
    std::ofstream aof_file_;
    bool aof_enabled_;

    std::string get_snapshot_path() const;
    std::string get_aof_path() const;
};

}
