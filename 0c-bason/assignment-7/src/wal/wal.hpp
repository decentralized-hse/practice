#pragma once

#include "../codec/bason_record.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace basonlite::wal {

class WalIterator {
public:
    bool valid() const { return false; }

    void next() {}

    uint64_t offset() const { return 0; }

    const codec::BasonRecord &record() const {
        static codec::BasonRecord b{};
        return b;
    }
};

class WalWriter {
public:
    static WalWriter open(const std::string &) { return WalWriter(); }

    uint64_t append(const codec::BasonRecord&) { return 0; }
    void checkpoint() {}
    void sync() {}
    void rotate(std::uint64_t) {}
};

class WalReader {
public:
    static WalReader open(const std::string &) { return WalReader(); }
    uint64_t recover() { return 0; }
    WalIterator scan(uint64_t) { return WalIterator(); }
};

void wal_truncate_before(const std::string& dir, std::uint64_t offset);

} // namespace basonlite::wal