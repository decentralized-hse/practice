#pragma once

#include "../codec/bason_codec.hpp"
#include <string>
#include <vector>

namespace bason {

class BasonRedisClient {
public:
    BasonRedisClient();
    ~BasonRedisClient();

    void connect(const std::string& host, uint16_t port);
    void disconnect();
    BasonRecord execute(const std::vector<std::string>& args);

    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    int64_t incr(const std::string& key);
    int64_t incrby(const std::string& key, int64_t delta);
    bool del(const std::string& key);
    bool exists(const std::string& key);

    size_t lpush(const std::string& key, const std::string& value);
    size_t rpush(const std::string& key, const std::string& value);
    size_t llen(const std::string& key);

    void hset(const std::string& key, const std::string& field, const std::string& value);
    std::string hget(const std::string& key, const std::string& field);

    void expire(const std::string& key, uint64_t ms);
    int64_t ttl(const std::string& key);

    std::vector<std::string> keys(const std::string& pattern);
    size_t dbsize();
    std::string ping();

private:
    int socket_fd_;
    bool connected_;

    BasonRecord send_command(const std::vector<std::string>& args);
};

}
