# BASONRedis Implementation Report

## Executive Summary

This report describes the implementation of BASONRedis, an in-memory data structure server that uses BASON as its wire protocol, command encoding, and persistence format.

## Implementation Overview

### Architecture

BASONRedis follows a modular architecture with clear separation of concerns:

1. **Data Store Layer** (`BasonRedisStore`) - Core in-memory hash map with type-specific operations
2. **Persistence Layer** (`PersistenceManager`) - AOF and snapshot management
3. **Network Layer** (`BasonRedisServer`) - TCP server with command processing
4. **Client Layer** (`BasonRedisClient`) - Client library for server communication

### Key Design Decisions

#### 1. In-Memory Storage

We use `std::unordered_map<std::string, Entry>` as the primary data structure where:
- **Key**: UTF-8 string (path format)
- **Value**: Entry containing a `BasonRecord` and optional expiry timestamp

This provides O(1) average-case lookup, insert, and delete operations.

#### 2. Type System

Each value is tagged with its BASON type:
- **Boolean** - for true/false/null values
- **String** - for UTF-8 text
- **Number** - for numeric values (stored as text per BASON spec)
- **Array** - for ordered sequences (using `std::vector<BasonRecord>`)
- **Object** - for key-value maps (using children with keys)

Type checking is enforced at runtime, throwing exceptions for type mismatches.

#### 3. TTL Implementation

We implement a hybrid expiry strategy:

**Lazy Expiry:**
- Check expiry on every read operation
- Remove expired keys immediately when accessed
- Zero overhead for keys without TTL

**Active Expiry:**
- Background thread runs every 1 second
- Scans all keys and removes expired ones
- Prevents memory leaks from never-accessed expired keys

#### 4. Thread Safety

All operations are protected by a single mutex (`std::mutex`):
- Simple and correct
- Sufficient for the assignment requirements
- Could be optimized with read-write locks or lock-free structures in production

#### 5. Persistence Strategy

**AOF (Append-Only File):**
- Every mutation command is appended to `appendonly.aof`
- Commands are BASON-encoded Arrays
- Replay on startup for crash recovery

**Snapshots:**
- Periodic full dumps to `dump.bason`
- Entire dataset as a single BASON Object
- Faster recovery than AOF alone

#### 6. Network Protocol

**Wire Format:**
- Commands: BASON Array `[command_name, arg1, arg2, ...]`
- Responses: BASON Record (type depends on command)

**Server Architecture:**
- Main thread accepts connections
- Each client handled in a separate pthread
- Non-blocking I/O not required for assignment

## Analysis Questions

### 1. AOF vs BASONCask

**Structural Differences:**

**BASONRedis AOF:**
- Stores **commands** (e.g., `["SET", "key", "value"]`)
- Replay executes commands to rebuild state
- Command-level granularity
- Supports all operations (including INCR, LPUSH, etc.)

**BASONCask:**
- Stores **data records** (key-value pairs)
- Replay loads records directly into hash map
- Record-level granularity
- Only supports PUT/DELETE semantics

**Why the difference?**

BASONRedis needs to preserve operation semantics (e.g., INCR vs SET), while BASONCask only needs final state.

### 2. Memory Limits and Eviction

**Current Behavior:**
- No memory limit
- No eviction policy
- Will consume all available RAM

**Proposed Solution:**

Implement LRU (Least Recently Used) eviction:

```cpp
class BasonRedisStore {
    size_t max_memory_;
    std::list<std::string> lru_list_;  // Most recent at front
    std::unordered_map<std::string, 
        std::list<std::string>::iterator> lru_map_;
    
    void evict_if_needed() {
        while (memory_usage() > max_memory_) {
            // Remove least recently used key
            std::string key = lru_list_.back();
            lru_list_.pop_back();
            data_.erase(key);
            lru_map_.erase(key);
        }
    }
    
    void touch(const std::string& key) {
        // Move to front of LRU list
        lru_list_.erase(lru_map_[key]);
        lru_list_.push_front(key);
        lru_map_[key] = lru_list_.begin();
    }
};
```

### 3. fsync Frequency Impact

**Measurement Plan:**

Test SET command latency with different fsync policies:

1. **sync-per-write**: fsync after every command
   - Latency: ~1-10ms (disk dependent)
   - Throughput: ~100-1000 ops/sec
   - Durability: Perfect

2. **sync-per-100**: fsync every 100 commands
   - Latency: ~10-100μs average
   - Throughput: ~10,000-100,000 ops/sec
   - Durability: May lose up to 100 commands

3. **sync-per-second**: fsync every 1 second
   - Latency: ~1-10μs average
   - Throughput: ~100,000-1,000,000 ops/sec
   - Durability: May lose up to 1 second of data

**Trade-off:**
- More frequent fsync = better durability, worse performance
- Less frequent fsync = better performance, worse durability