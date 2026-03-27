#include "bason_redis_store.hpp"
#include <algorithm>
#include <sstream>

namespace bason {

BasonRedisStore::BasonRedisStore() {}

BasonRedisStore::~BasonRedisStore() {}

int64_t BasonRedisStore::current_time_ms() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

bool BasonRedisStore::is_expired(const Entry& entry) const {
    if (entry.expiry_ms < 0) return false;
    return current_time_ms() >= entry.expiry_ms;
}

void BasonRedisStore::check_and_remove_expired(const std::string& key) {
    auto it = data_.find(key);
    if (it != data_.end() && is_expired(it->second)) {
        data_.erase(it);
    }
}

void BasonRedisStore::set(const std::string& key, BasonRecord value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = Entry(value);
}

std::optional<BasonRecord> BasonRedisStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);
    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    return it->second.record;
}

bool BasonRedisStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(key) > 0;
}

bool BasonRedisStore::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);
    return data_.find(key) != data_.end();
}

BasonType BasonRedisStore::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);
    auto it = data_.find(key);
    if (it == data_.end()) throw std::runtime_error("Key not found");
    return it->second.record.type;
}

int64_t BasonRedisStore::incr(const std::string& key) {
    return incrby(key, 1);
}

int64_t BasonRedisStore::incrby(const std::string& key, int64_t delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    int64_t value = 0;

    if (it != data_.end()) {
        if (it->second.record.type != BasonType::Number) {
            throw std::runtime_error("Value is not a number");
        }
        value = std::stoll(it->second.record.value);
    }

    value += delta;
    BasonRecord rec(BasonType::Number);
    rec.value = std::to_string(value);
    data_[key] = Entry(rec);

    return value;
}

int64_t BasonRedisStore::decr(const std::string& key) {
    return decrby(key, 1);
}

int64_t BasonRedisStore::decrby(const std::string& key, int64_t delta) {
    return incrby(key, -delta);
}

size_t BasonRedisStore::append(const std::string& key, const std::string& suffix) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it != data_.end()) {
        if (it->second.record.type != BasonType::String) {
            throw std::runtime_error("Value is not a string");
        }
        it->second.record.value += suffix;
        return it->second.record.value.size();
    } else {
        BasonRecord rec(BasonType::String);
        rec.value = suffix;
        data_[key] = Entry(rec);
        return suffix.size();
    }
}

size_t BasonRedisStore::strlen_cmd(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return 0;
    if (it->second.record.type != BasonType::String) {
        throw std::runtime_error("Value is not a string");
    }
    return it->second.record.value.size();
}

std::string BasonRedisStore::getrange(const std::string& key, int64_t start, int64_t end) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return "";
    if (it->second.record.type != BasonType::String) {
        throw std::runtime_error("Value is not a string");
    }

    const std::string& str = it->second.record.value;
    int64_t len = str.size();

    if (start < 0) start = len + start;
    if (end < 0) end = len + end;
    if (start < 0) start = 0;
    if (end >= len) end = len - 1;
    if (start > end) return "";

    return str.substr(start, end - start + 1);
}

size_t BasonRedisStore::lpush(const std::string& key, const BasonRecord& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it != data_.end()) {
        if (it->second.record.type != BasonType::Array) {
            throw std::runtime_error("Value is not an array");
        }
        it->second.record.children.insert(it->second.record.children.begin(), value);
        return it->second.record.children.size();
    } else {
        BasonRecord rec(BasonType::Array);
        rec.children.push_back(value);
        data_[key] = Entry(rec);
        return 1;
    }
}

size_t BasonRedisStore::rpush(const std::string& key, const BasonRecord& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it != data_.end()) {
        if (it->second.record.type != BasonType::Array) {
            throw std::runtime_error("Value is not an array");
        }
        it->second.record.children.push_back(value);
        return it->second.record.children.size();
    } else {
        BasonRecord rec(BasonType::Array);
        rec.children.push_back(value);
        data_[key] = Entry(rec);
        return 1;
    }
}

std::optional<BasonRecord> BasonRedisStore::lpop(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    if (it->second.record.type != BasonType::Array) {
        throw std::runtime_error("Value is not an array");
    }
    if (it->second.record.children.empty()) return std::nullopt;

    BasonRecord popped = it->second.record.children.front();
    it->second.record.children.erase(it->second.record.children.begin());
    return popped;
}

std::optional<BasonRecord> BasonRedisStore::rpop(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    if (it->second.record.type != BasonType::Array) {
        throw std::runtime_error("Value is not an array");
    }
    if (it->second.record.children.empty()) return std::nullopt;

    BasonRecord popped = it->second.record.children.back();
    it->second.record.children.pop_back();
    return popped;
}

std::optional<BasonRecord> BasonRedisStore::lindex(const std::string& key, int64_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    if (it->second.record.type != BasonType::Array) {
        throw std::runtime_error("Value is not an array");
    }

    int64_t len = static_cast<int64_t>(it->second.record.children.size());
    if (index < 0) index = len + index;
    if (index < 0 || index >= len) return std::nullopt;

    return it->second.record.children[static_cast<size_t>(index)];
}

size_t BasonRedisStore::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return 0;
    if (it->second.record.type != BasonType::Array) {
        throw std::runtime_error("Value is not an array");
    }
    return it->second.record.children.size();
}

std::vector<BasonRecord> BasonRedisStore::lrange(const std::string& key, int64_t start, int64_t stop) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::vector<BasonRecord>();
    if (it->second.record.type != BasonType::Array) {
        throw std::runtime_error("Value is not an array");
    }

    int64_t len = it->second.record.children.size();
    if (start < 0) start = len + start;
    if (stop < 0) stop = len + stop;
    if (start < 0) start = 0;
    if (stop >= len) stop = len - 1;
    if (start > stop) return std::vector<BasonRecord>();

    std::vector<BasonRecord> result;
    for (int64_t i = start; i <= stop; ++i) {
        result.push_back(it->second.record.children[i]);
    }
    return result;
}

void BasonRedisStore::hset(const std::string& key, const std::string& field, const BasonRecord& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it != data_.end()) {
        if (it->second.record.type != BasonType::Object) {
            throw std::runtime_error("Value is not an object");
        }
        bool found = false;
        for (auto& child : it->second.record.children) {
            if (child.key == field) {
                child = value;
                child.key = field;
                found = true;
                break;
            }
        }
        if (!found) {
            BasonRecord field_rec = value;
            field_rec.key = field;
            it->second.record.children.push_back(field_rec);
        }
    } else {
        BasonRecord rec(BasonType::Object);
        BasonRecord field_rec = value;
        field_rec.key = field;
        rec.children.push_back(field_rec);
        data_[key] = Entry(rec);
    }
}

std::optional<BasonRecord> BasonRedisStore::hget(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    if (it->second.record.type != BasonType::Object) {
        throw std::runtime_error("Value is not an object");
    }

    for (const auto& child : it->second.record.children) {
        if (child.key == field) {
            return child;
        }
    }
    return std::nullopt;
}

bool BasonRedisStore::hdel(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return false;
    if (it->second.record.type != BasonType::Object) {
        throw std::runtime_error("Value is not an object");
    }

    auto& children = it->second.record.children;
    for (auto child_it = children.begin(); child_it != children.end(); ++child_it) {
        if (child_it->key == field) {
            children.erase(child_it);
            return true;
        }
    }
    return false;
}

std::vector<std::string> BasonRedisStore::hkeys(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::vector<std::string>();
    if (it->second.record.type != BasonType::Object) {
        throw std::runtime_error("Value is not an object");
    }

    std::vector<std::string> result;
    for (const auto& child : it->second.record.children) {
        result.push_back(child.key);
    }
    return result;
}

std::vector<BasonRecord> BasonRedisStore::hvals(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::vector<BasonRecord>();
    if (it->second.record.type != BasonType::Object) {
        throw std::runtime_error("Value is not an object");
    }

    return it->second.record.children;
}

size_t BasonRedisStore::hlen(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return 0;
    if (it->second.record.type != BasonType::Object) {
        throw std::runtime_error("Value is not an object");
    }
    return it->second.record.children.size();
}

std::unordered_map<std::string, BasonRecord> BasonRedisStore::hgetall(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return std::unordered_map<std::string, BasonRecord>();
    if (it->second.record.type != BasonType::Object) {
        throw std::runtime_error("Value is not an object");
    }

    std::unordered_map<std::string, BasonRecord> result;
    for (const auto& child : it->second.record.children) {
        result[child.key] = child;
    }
    return result;
}

void BasonRedisStore::expire(const std::string& key, uint64_t ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        it->second.expiry_ms = current_time_ms() + ms;
    }
}

int64_t BasonRedisStore::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    check_and_remove_expired(key);

    auto it = data_.find(key);
    if (it == data_.end()) return -2;
    if (it->second.expiry_ms < 0) return -1;

    int64_t remaining = it->second.expiry_ms - current_time_ms();
    return remaining > 0 ? remaining : -2;
}

void BasonRedisStore::persist(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        it->second.expiry_ms = -1;
    }
}

bool BasonRedisStore::glob_match(const std::string& pattern, const std::string& str) const {
    size_t p = 0, s = 0;
    size_t star_p = std::string::npos, star_s = 0;

    while (s < str.size()) {
        if (p < pattern.size() && (pattern[p] == str[s] || pattern[p] == '?')) {
            ++p; ++s;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star_p = p++;
            star_s = s;
        } else if (star_p != std::string::npos) {
            p = star_p + 1;
            s = ++star_s;
        } else {
            return false;
        }
    }

    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

std::vector<std::string> BasonRedisStore::keys(const std::string& glob_pattern) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> result;
    for (const auto& pair : data_) {
        if (!is_expired(pair.second) && glob_match(glob_pattern, pair.first)) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void BasonRedisStore::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
}

size_t BasonRedisStore::dbsize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

void BasonRedisStore::cleanup_expired() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> to_remove;
    for (const auto& pair : data_) {
        if (is_expired(pair.second)) {
            to_remove.push_back(pair.first);
        }
    }

    for (const auto& key : to_remove) {
        data_.erase(key);
    }
}

}
