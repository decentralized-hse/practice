#include "bason_redis_server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace bason {

BasonRedisServer::BasonRedisServer(const std::string& data_dir)
    : running_(false) {
    persistence_ = std::make_unique<PersistenceManager>(data_dir);
    persistence_->recover(store_);
}

BasonRedisServer::~BasonRedisServer() {
    shutdown();
}

void BasonRedisServer::expiry_worker() {
    while (running_) {
        store_.cleanup_expired();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

BasonRecord BasonRedisServer::parse_command(const std::vector<std::string>& args) {
    BasonRecord cmd(BasonType::Array);
    for (const auto& arg : args) {
        BasonRecord arg_rec(BasonType::String);
        arg_rec.value = arg;
        cmd.children.push_back(arg_rec);
    }
    return cmd;
}

static void try_log(PersistenceManager* pm, const BasonRecord& cmd) {
    try { pm->log_command(cmd); } catch (...) {}
}

BasonRecord BasonRedisServer::execute(const BasonRecord& command) {
    if (command.type != BasonType::Array || command.children.empty()) {
        BasonRecord error(BasonType::String);
        error.value = "ERR invalid command format";
        return error;
    }

    std::string cmd = command.children[0].value;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    try {
        if (cmd == "SET" && command.children.size() >= 3) {
            std::string key = command.children[1].value;
            BasonRecord value(BasonType::String);
            value.value = command.children[2].value;
            store_.set(key, value);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::String);
            response.value = "OK";
            return response;
        }
        else if (cmd == "GET" && command.children.size() >= 2) {
            std::string key = command.children[1].value;
            auto value = store_.get(key);
            if (value) return *value;
            BasonRecord null_resp(BasonType::Boolean);
            null_resp.value = "";
            return null_resp;
        }
        else if (cmd == "DEL" && command.children.size() >= 2) {
            bool deleted = store_.del(command.children[1].value);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::Number);
            response.value = deleted ? "1" : "0";
            return response;
        }
        else if (cmd == "EXISTS" && command.children.size() >= 2) {
            bool exists = store_.exists(command.children[1].value);
            BasonRecord response(BasonType::Number);
            response.value = exists ? "1" : "0";
            return response;
        }
        else if (cmd == "INCR" && command.children.size() >= 2) {
            int64_t value = store_.incr(command.children[1].value);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::Number);
            response.value = std::to_string(value);
            return response;
        }
        else if (cmd == "INCRBY" && command.children.size() >= 3) {
            int64_t delta = std::stoll(command.children[2].value);
            int64_t value = store_.incrby(command.children[1].value, delta);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::Number);
            response.value = std::to_string(value);
            return response;
        }
        else if (cmd == "LPUSH" && command.children.size() >= 3) {
            BasonRecord value(BasonType::String);
            value.value = command.children[2].value;
            size_t len = store_.lpush(command.children[1].value, value);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::Number);
            response.value = std::to_string(len);
            return response;
        }
        else if (cmd == "RPUSH" && command.children.size() >= 3) {
            BasonRecord value(BasonType::String);
            value.value = command.children[2].value;
            size_t len = store_.rpush(command.children[1].value, value);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::Number);
            response.value = std::to_string(len);
            return response;
        }
        else if (cmd == "LLEN" && command.children.size() >= 2) {
            size_t len = store_.llen(command.children[1].value);
            BasonRecord response(BasonType::Number);
            response.value = std::to_string(len);
            return response;
        }
        else if (cmd == "HSET" && command.children.size() >= 4) {
            BasonRecord value(BasonType::String);
            value.value = command.children[3].value;
            store_.hset(command.children[1].value, command.children[2].value, value);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::Number);
            response.value = "1";
            return response;
        }
        else if (cmd == "HGET" && command.children.size() >= 3) {
            auto value = store_.hget(command.children[1].value, command.children[2].value);
            if (value) return *value;
            BasonRecord null_resp(BasonType::Boolean);
            null_resp.value = "";
            return null_resp;
        }
        else if (cmd == "EXPIRE" && command.children.size() >= 3) {
            uint64_t ms = std::stoull(command.children[2].value);
            store_.expire(command.children[1].value, ms);
            try_log(persistence_.get(), command);
            BasonRecord response(BasonType::Number);
            response.value = "1";
            return response;
        }
        else if (cmd == "TTL" && command.children.size() >= 2) {
            int64_t ttl = store_.ttl(command.children[1].value);
            BasonRecord response(BasonType::Number);
            response.value = std::to_string(ttl);
            return response;
        }
        else if (cmd == "KEYS" && command.children.size() >= 2) {
            auto keys = store_.keys(command.children[1].value);
            BasonRecord response(BasonType::Array);
            for (const auto& k : keys) {
                BasonRecord key_rec(BasonType::String);
                key_rec.value = k;
                response.children.push_back(key_rec);
            }
            return response;
        }
        else if (cmd == "DBSIZE") {
            BasonRecord response(BasonType::Number);
            response.value = std::to_string(store_.dbsize());
            return response;
        }
        else if (cmd == "PING") {
            BasonRecord response(BasonType::String);
            response.value = "PONG";
            return response;
        }
        else {
            BasonRecord error(BasonType::String);
            error.value = "ERR unknown command '" + cmd + "'";
            return error;
        }
    } catch (const std::exception& e) {
        BasonRecord error(BasonType::String);
        error.value = std::string("ERR ") + e.what();
        return error;
    }
}

BasonRecord BasonRedisServer::execute_command(const std::vector<std::string>& args) {
    BasonRecord cmd = parse_command(args);
    return execute(cmd);
}

void BasonRedisServer::handle_client(int client_fd) {
    char buffer[4096];

    while (running_) {
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) break;

        try {
            auto records = BasonCodec::decode_all(
                reinterpret_cast<uint8_t*>(buffer), bytes_read);

            for (const auto& cmd : records) {
                BasonRecord response = execute(cmd);
                auto encoded = BasonCodec::encode(response);
                send(client_fd, encoded.data(), encoded.size(), 0);
            }
        } catch (const std::exception& e) {
            BasonRecord error(BasonType::String);
            error.value = std::string("ERR ") + e.what();
            auto encoded = BasonCodec::encode(error);
            send(client_fd, encoded.data(), encoded.size(), 0);
        }
    }

    close(client_fd);
}

void BasonRedisServer::listen(uint16_t port) {
    running_ = true;

    std::thread([this]{ expiry_worker(); }).detach();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to bind to port");
    }

    if (::listen(server_fd, 10) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "BASONRedis server listening on port " << port << std::endl;

    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }

        std::thread([this, client_fd]{ handle_client(client_fd); }).detach();
    }

    close(server_fd);
}

void BasonRedisServer::shutdown() {
    if (running_) {
        running_ = false;

        if (persistence_) {
            persistence_->sync();
            persistence_->snapshot(store_, "");
        }
    }
}

}
