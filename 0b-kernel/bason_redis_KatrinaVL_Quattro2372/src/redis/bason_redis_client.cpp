#include "bason_redis_client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

namespace bason {

BasonRedisClient::BasonRedisClient() : socket_fd_(-1), connected_(false) {}

BasonRedisClient::~BasonRedisClient() {
    disconnect();
}

void BasonRedisClient::connect(const std::string& host, uint16_t port) {
    if (connected_) {
        throw std::runtime_error("Already connected");
    }

    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        close(socket_fd_);
        throw std::runtime_error("Invalid address");
    }

    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd_);
        throw std::runtime_error("Connection failed");
    }

    connected_ = true;
}

void BasonRedisClient::disconnect() {
    if (connected_) {
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
    }
}

BasonRecord BasonRedisClient::send_command(const std::vector<std::string>& args) {
    if (!connected_) {
        throw std::runtime_error("Not connected");
    }

    BasonRecord cmd(BasonType::Array);
    for (const auto& arg : args) {
        BasonRecord arg_rec(BasonType::String);
        arg_rec.value = arg;
        cmd.children.push_back(arg_rec);
    }

    auto encoded = BasonCodec::encode(cmd);
    ssize_t sent = send(socket_fd_, encoded.data(), encoded.size(), 0);
    if (sent < 0) {
        throw std::runtime_error("Failed to send command");
    }

    char buffer[4096];
    ssize_t received = recv(socket_fd_, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        throw std::runtime_error("Failed to receive response");
    }

    auto result = BasonCodec::decode(reinterpret_cast<uint8_t*>(buffer), received);
    return result.first;
}

BasonRecord BasonRedisClient::execute(const std::vector<std::string>& args) {
    return send_command(args);
}

void BasonRedisClient::set(const std::string& key, const std::string& value) {
    send_command({"SET", key, value});
}

std::string BasonRedisClient::get(const std::string& key) {
    auto response = send_command({"GET", key});
    if (response.type == BasonType::Boolean && response.value.empty()) {
        return "";
    }
    return response.value;
}

int64_t BasonRedisClient::incr(const std::string& key) {
    auto response = send_command({"INCR", key});
    return std::stoll(response.value);
}

int64_t BasonRedisClient::incrby(const std::string& key, int64_t delta) {
    auto response = send_command({"INCRBY", key, std::to_string(delta)});
    return std::stoll(response.value);
}

bool BasonRedisClient::del(const std::string& key) {
    auto response = send_command({"DEL", key});
    return response.value == "1";
}

bool BasonRedisClient::exists(const std::string& key) {
    auto response = send_command({"EXISTS", key});
    return response.value == "1";
}

size_t BasonRedisClient::lpush(const std::string& key, const std::string& value) {
    auto response = send_command({"LPUSH", key, value});
    return std::stoull(response.value);
}

size_t BasonRedisClient::rpush(const std::string& key, const std::string& value) {
    auto response = send_command({"RPUSH", key, value});
    return std::stoull(response.value);
}

size_t BasonRedisClient::llen(const std::string& key) {
    auto response = send_command({"LLEN", key});
    return std::stoull(response.value);
}

void BasonRedisClient::hset(const std::string& key, const std::string& field, const std::string& value) {
    send_command({"HSET", key, field, value});
}

std::string BasonRedisClient::hget(const std::string& key, const std::string& field) {
    auto response = send_command({"HGET", key, field});
    if (response.type == BasonType::Boolean && response.value.empty()) {
        return "";
    }
    return response.value;
}

void BasonRedisClient::expire(const std::string& key, uint64_t ms) {
    send_command({"EXPIRE", key, std::to_string(ms)});
}

int64_t BasonRedisClient::ttl(const std::string& key) {
    auto response = send_command({"TTL", key});
    return std::stoll(response.value);
}

std::vector<std::string> BasonRedisClient::keys(const std::string& pattern) {
    auto response = send_command({"KEYS", pattern});
    std::vector<std::string> result;
    for (const auto& child : response.children) {
        result.push_back(child.value);
    }
    return result;
}

size_t BasonRedisClient::dbsize() {
    auto response = send_command({"DBSIZE"});
    return std::stoull(response.value);
}

std::string BasonRedisClient::ping() {
    auto response = send_command({"PING"});
    return response.value;
}

} // namespace bason
