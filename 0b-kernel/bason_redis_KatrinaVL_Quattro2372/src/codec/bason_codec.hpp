#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace bason {

enum class BasonType {
    Boolean,
    Array,
    String,
    Object,
    Number
};

struct BasonRecord {
    BasonType type;
    std::string key;
    std::string value;
    std::vector<BasonRecord> children;
    
    BasonRecord() : type(BasonType::String) {}
    BasonRecord(BasonType t) : type(t) {}
    BasonRecord(BasonType t, const std::string& k, const std::string& v) 
        : type(t), key(k), value(v) {}
};

class Ron64 {
public:
    static std::string encode(uint64_t value) {
        throw std::runtime_error("Not implemented");
    }
    
    static uint64_t decode(const std::string& s) {
        throw std::runtime_error("Not implemented");
    }
};

class Path {
public:
    static std::string join(const std::vector<std::string>& segments) {
        throw std::runtime_error("Not implemented");
    }
    
    static std::vector<std::string> split(const std::string& path) {
        throw std::runtime_error("Not implemented");
    }
    
    static std::string parent(const std::string& path) {
        throw std::runtime_error("Not implemented");
    }
    
    static std::string basename(const std::string& path) {
        throw std::runtime_error("Not implemented");
    }
};

class BasonCodec {
public:
    static std::vector<uint8_t> encode(const BasonRecord& record) {
        throw std::runtime_error("Not implemented");
    }
    
    static std::pair<BasonRecord, size_t> decode(const uint8_t* data, size_t len) {
        throw std::runtime_error("Not implemented");
    }
    
    static std::vector<BasonRecord> decode_all(const uint8_t* data, size_t len) {
        throw std::runtime_error("Not implemented");
    }
    
    static std::vector<uint8_t> json_to_bason(const std::string& json) {
        throw std::runtime_error("Not implemented");
    }
    
    static std::string bason_to_json(const uint8_t* data, size_t len) {
        throw std::runtime_error("Not implemented");
    }
};

}
