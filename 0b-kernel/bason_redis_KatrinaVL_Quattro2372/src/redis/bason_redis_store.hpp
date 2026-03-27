#pragma once

#include "../codec/bason_codec.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <memory>
#include <optional>

namespace bason {

class BasonRedisStore {
public:
    BasonRedisStore();
    ~BasonRedisStore();

    void set(const std::string& key, BasonRecord value);
    std::optional<BasonRecord> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    BasonType type(const std::string& key);

    int64_t incr(const std::string& key);
    int64_t incrby(const std::string& key, int64_t delta);
    int64_t decr(const std::string& key);
    int64_t decrby(const std::string& key, int64_t delta);

    size_t append(const std::string& key, const std::string& suffix);
    size_t strlen_cmd(const std::string& key);
    std::string getrange(const std::string& key, int64_t start, int64_t end);

    size_t lpush(const std::string& key, const BasonRecord& value);
    size_t rpush(const std::string& key, const BasonRecord& value);
    std::optional<BasonRecord> lpop(const std::string& key);
    std::optional<BasonRecord> rpop(const std::string& key);
    std::optional<BasonRecord> lindex(const std::string& key, int64_t index);
    size_t llen(const std::string& key);
    std::vector<BasonRecord> lrange(const std::string& key, int64_t start, int64_t stop);

    void hset(const std::string& key, const std::string& field, const BasonRecord& value);
    std::optional<BasonRecord> hget(const std::string& key, const std::string& field);
    bool hdel(const std::string& key, const std::string& field);
    std::vector<std::string> hkeys(const std::string& key);
    std::vector<BasonRecord> hvals(const std::string& key);
    size_t hlen(const std::string& key);
    std::unordered_map<std::string, BasonRecord> hgetall(const std::string& key);

    void expire(const std::string& key, uint64_t ms);
    int64_t ttl(const std::string& key);
    void persist(const std::string& key);

    std::vector<std::string> keys(const std::string& glob_pattern);

    void clear();
    size_t dbsize() const;

    void cleanup_expired();

private:
    struct Entry {
        BasonRecord record;
        int64_t expiry_ms;

        Entry() : expiry_ms(-1) {}
        Entry(const BasonRecord& r) : record(r), expiry_ms(-1) {}
    };
    
    std::unordered_map<std::string, Entry> data_;
    mutable std::mutex mutex_;
    
    bool is_expired(const Entry& entry) const;
    void check_and_remove_expired(const std::string& key);
    bool glob_match(const std::string& pattern, const std::string& str) const;
    int64_t current_time_ms() const;
};

}
