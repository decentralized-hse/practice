#pragma once

#include "bason_redis_store.hpp"
#include "persistence_manager.hpp"
#include <string>
#include <atomic>
#include <memory>
#include <thread>

namespace bason {

class BasonRedisServer {
public:
    BasonRedisServer(const std::string& data_dir = "./data");
    ~BasonRedisServer();

    void listen(uint16_t port);
    void shutdown();
    BasonRecord execute_command(const std::vector<std::string>& args);

    void handle_client(int client_fd);
    void expiry_worker();

private:
    BasonRedisStore store_;
    std::unique_ptr<PersistenceManager> persistence_;
    std::atomic<bool> running_;

    BasonRecord parse_command(const std::vector<std::string>& args);
    BasonRecord execute(const BasonRecord& command);
};

}
