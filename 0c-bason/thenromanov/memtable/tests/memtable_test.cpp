#include "codec/record.hpp"

#include "memtable/memtable.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace bason_db;

TEST(MemtableTest, PutAndGet) {
    auto memtable = Memtable{};
    memtable.put("key1", make_string_record("key1", "value1"), 1);
    memtable.put("key2", make_string_record("key2", "value2"), 2);

    auto record1 = memtable.get("key1");
    ASSERT_TRUE(record1.has_value());
    EXPECT_EQ(record1->first.value, "value1");
    EXPECT_EQ(record1->second, 1U);

    auto record2 = memtable.get("key2");
    ASSERT_TRUE(record2.has_value());
    EXPECT_EQ(record2->first.value, "value2");
}

TEST(MemtableTest, UpdateKeepsHighestOffset) {
    auto memtable = Memtable{};
    memtable.put("key", make_string_record("key", "v1"), 1);
    memtable.put("key", make_string_record("key", "v2"), 5);

    auto record = memtable.get("key");
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->first.value, "v2");
    EXPECT_EQ(record->second, 5U);
}

TEST(MemtableTest, MissingKey) {
    auto memtable = Memtable{};
    memtable.put("a", make_string_record("a", "1"), 1);
    EXPECT_FALSE(memtable.get("b").has_value());
}

TEST(MemtableTest, SortedIteration) {
    auto memtable = Memtable{};
    memtable.put("c", make_string_record("c", "3"), 3);
    memtable.put("a", make_string_record("a", "1"), 1);
    memtable.put("b", make_string_record("b", "2"), 2);

    auto it = memtable.scan();
    ASSERT_TRUE(it->valid());
    EXPECT_EQ(it->key(), "a");
    it->next();
    EXPECT_EQ(it->key(), "b");
    it->next();
    EXPECT_EQ(it->key(), "c");
    it->next();
    EXPECT_FALSE(it->valid());
}

TEST(MemtableTest, RangeScan) {
    auto memtable = Memtable{};
    for (char c = 'a'; c <= 'e'; ++c) {
        auto key = std::string(1, c);
        memtable.put(key, make_string_record(key, key), static_cast<uint64_t>(c));
    }

    auto it = memtable.scan("b", "d");
    ASSERT_TRUE(it->valid());
    EXPECT_EQ(it->key(), "b");
    it->next();
    EXPECT_EQ(it->key(), "c");
    it->next();
    EXPECT_FALSE(it->valid()); // "d" is exclusive
}

TEST(MemtableTest, Freeze) {
    auto memtable = Memtable{};
    memtable.put("x", make_string_record("x", "1"), 1);
    memtable.put("y", make_string_record("y", "2"), 2);

    auto frozen = memtable.freeze();
    EXPECT_EQ(memtable.count(), 0U);

    auto record = frozen->get("x");
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->first.value, "1");

    memtable.put("z", make_string_record("z", "3"), 3);
    EXPECT_EQ(memtable.count(), 1U);
    EXPECT_FALSE(frozen->get("z").has_value());
}

TEST(MemtableTest, MemoryUsage) {
    auto memtable = Memtable{};
    EXPECT_EQ(memtable.memory_usage(), 0U);
    memtable.put("key", make_string_record("key", "value"), 1);
    EXPECT_GT(memtable.memory_usage(), 0U);
}
