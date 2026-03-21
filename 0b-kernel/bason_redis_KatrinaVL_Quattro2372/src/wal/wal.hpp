#pragma once

#include "../codec/bason_codec.hpp"
#include <string>
#include <cstdint>

namespace bason {

// Write-Ahead Log (stub - Assignment 2)
class WalWriter {
public:
    static WalWriter open(const std::string& dir) {
        throw std::runtime_error("Not implemented");
    }
    
    uint64_t append(const BasonRecord& record) {
        throw std::runtime_error("Not implemented");
    }
    
    void checkpoint() {
        throw std::runtime_error("Not implemented");
    }
    
    void sync() {
        throw std::runtime_error("Not implemented");
    }
    
    void rotate(uint64_t max_segment_size) {
        throw std::runtime_error("Not implemented");
    }
};

class WalIterator {
public:
    bool valid() const {
        throw std::runtime_error("Not implemented");
    }
    
    void next() {
        throw std::runtime_error("Not implemented");
    }
    
    uint64_t offset() const {
        throw std::runtime_error("Not implemented");
    }
    
    const BasonRecord& record() const {
        throw std::runtime_error("Not implemented");
    }
};

class WalReader {
public:
    static WalReader open(const std::string& dir) {
        throw std::runtime_error("Not implemented");
    }
    
    uint64_t recover() {
        throw std::runtime_error("Not implemented");
    }
    
    WalIterator scan(uint64_t from_offset) {
        throw std::runtime_error("Not implemented");
    }
};

} // namespace bason
