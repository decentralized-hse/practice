#include <gtest/gtest.h>
#include "../src/redis/bason_redis_store.hpp"

using namespace bason;

class StoreTest : public ::testing::Test {
protected:
    BasonRedisStore store;

    BasonRecord make_string(const std::string& val) {
        BasonRecord r(BasonType::String);
        r.value = val;
        return r;
    }

    BasonRecord make_number(int64_t val) {
        BasonRecord r(BasonType::Number);
        r.value = std::to_string(val);
        return r;
    }
};

TEST_F(StoreTest, SetAndGet) {
    store.set("key1", make_string("hello"));
    auto result = store.get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "hello");
    EXPECT_EQ(result->type, BasonType::String);
}

TEST_F(StoreTest, GetMissingKeyReturnsNull) {
    EXPECT_FALSE(store.get("nonexistent").has_value());
}

TEST_F(StoreTest, ExistsReturnsTrueForPresentKey) {
    store.set("k", make_string("v"));
    EXPECT_TRUE(store.exists("k"));
}

TEST_F(StoreTest, ExistsReturnsFalseForMissingKey) {
    EXPECT_FALSE(store.exists("missing"));
}

TEST_F(StoreTest, DelRemovesKey) {
    store.set("k", make_string("v"));
    EXPECT_TRUE(store.del("k"));
    EXPECT_FALSE(store.exists("k"));
}

TEST_F(StoreTest, DelReturnsFalseForMissingKey) {
    EXPECT_FALSE(store.del("missing"));
}

TEST_F(StoreTest, DbsizeReflectsCount) {
    EXPECT_EQ(store.dbsize(), 0u);
    store.set("a", make_string("1"));
    store.set("b", make_string("2"));
    EXPECT_EQ(store.dbsize(), 2u);
}

TEST_F(StoreTest, ClearRemovesAllKeys) {
    store.set("a", make_string("1"));
    store.set("b", make_string("2"));
    store.clear();
    EXPECT_EQ(store.dbsize(), 0u);
}

TEST_F(StoreTest, TypeReturnsCorrectType) {
    store.set("s", make_string("hello"));
    EXPECT_EQ(store.type("s"), BasonType::String);

    store.set("n", make_number(42));
    EXPECT_EQ(store.type("n"), BasonType::Number);
}

TEST_F(StoreTest, IncrCreatesKeyWithOne) {
    EXPECT_EQ(store.incr("counter"), 1);
}

TEST_F(StoreTest, IncrIncrementsExistingKey) {
    store.incr("counter");
    EXPECT_EQ(store.incr("counter"), 2);
}

TEST_F(StoreTest, IncrbyAddsPositiveDelta) {
    store.incr("counter");
    EXPECT_EQ(store.incrby("counter", 5), 6);
}

TEST_F(StoreTest, DecrDecrementsKey) {
    store.incrby("counter", 10);
    EXPECT_EQ(store.decr("counter"), 9);
}

TEST_F(StoreTest, DecrbySubtractsDelta) {
    store.incrby("counter", 10);
    EXPECT_EQ(store.decrby("counter", 3), 7);
}

TEST_F(StoreTest, IncrOnNonNumberThrows) {
    store.set("s", make_string("hello"));
    EXPECT_THROW(store.incr("s"), std::runtime_error);
}

TEST_F(StoreTest, AppendToExistingString) {
    store.set("s", make_string("hello"));
    size_t len = store.append("s", " world");
    EXPECT_EQ(len, 11u);
    auto result = store.get("s");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "hello world");
}

TEST_F(StoreTest, AppendToMissingKeyCreatesString) {
    size_t len = store.append("s", "hello");
    EXPECT_EQ(len, 5u);
    auto result = store.get("s");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "hello");
}

TEST_F(StoreTest, StrlenReturnsLength) {
    store.set("s", make_string("hello"));
    EXPECT_EQ(store.strlen_cmd("s"), 5u);
}

TEST_F(StoreTest, StrlenMissingKeyReturnsZero) {
    EXPECT_EQ(store.strlen_cmd("missing"), 0u);
}

TEST_F(StoreTest, GetrangeReturnsSubstring) {
    store.set("s", make_string("hello world"));
    EXPECT_EQ(store.getrange("s", 0, 4), "hello");
    EXPECT_EQ(store.getrange("s", 6, 10), "world");
}

TEST_F(StoreTest, GetrangeNegativeIndices) {
    store.set("s", make_string("hello"));
    EXPECT_EQ(store.getrange("s", -3, -1), "llo");
}

TEST_F(StoreTest, LpushAddsToFront) {
    store.lpush("list", make_string("b"));
    store.lpush("list", make_string("a"));
    EXPECT_EQ(store.llen("list"), 2u);
    auto elem = store.lindex("list", 0);
    ASSERT_TRUE(elem.has_value());
    EXPECT_EQ(elem->value, "a");
}

TEST_F(StoreTest, RpushAddsToBack) {
    store.rpush("list", make_string("a"));
    store.rpush("list", make_string("b"));
    EXPECT_EQ(store.llen("list"), 2u);
    auto elem = store.lindex("list", 1);
    ASSERT_TRUE(elem.has_value());
    EXPECT_EQ(elem->value, "b");
}

TEST_F(StoreTest, LlenMissingKeyReturnsZero) {
    EXPECT_EQ(store.llen("missing"), 0u);
}

TEST_F(StoreTest, LindexOutOfBoundsReturnsNull) {
    store.rpush("list", make_string("a"));
    EXPECT_FALSE(store.lindex("list", 5).has_value());
}

TEST_F(StoreTest, LindexNegativeIndex) {
    store.rpush("list", make_string("a"));
    store.rpush("list", make_string("b"));
    auto elem = store.lindex("list", -1);
    ASSERT_TRUE(elem.has_value());
    EXPECT_EQ(elem->value, "b");
}

TEST_F(StoreTest, LrangeReturnsSublist) {
    store.rpush("list", make_string("a"));
    store.rpush("list", make_string("b"));
    store.rpush("list", make_string("c"));
    auto result = store.lrange("list", 0, 1);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].value, "a");
    EXPECT_EQ(result[1].value, "b");
}

TEST_F(StoreTest, LpushOnNonArrayThrows) {
    store.set("s", make_string("hello"));
    EXPECT_THROW(store.lpush("s", make_string("x")), std::runtime_error);
}

TEST_F(StoreTest, HsetAndHget) {
    store.hset("hash", "field1", make_string("value1"));
    auto result = store.hget("hash", "field1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "value1");
}

TEST_F(StoreTest, HgetMissingFieldReturnsNull) {
    store.hset("hash", "f", make_string("v"));
    EXPECT_FALSE(store.hget("hash", "missing").has_value());
}

TEST_F(StoreTest, HgetMissingKeyReturnsNull) {
    EXPECT_FALSE(store.hget("missing", "field").has_value());
}

TEST_F(StoreTest, HdelRemovesField) {
    store.hset("hash", "f", make_string("v"));
    EXPECT_TRUE(store.hdel("hash", "f"));
    EXPECT_FALSE(store.hget("hash", "f").has_value());
}

TEST_F(StoreTest, HdelReturnsFalseForMissingField) {
    store.hset("hash", "f", make_string("v"));
    EXPECT_FALSE(store.hdel("hash", "missing"));
}

TEST_F(StoreTest, HlenReturnsFieldCount) {
    store.hset("hash", "f1", make_string("v1"));
    store.hset("hash", "f2", make_string("v2"));
    EXPECT_EQ(store.hlen("hash"), 2u);
}

TEST_F(StoreTest, HkeysReturnsAllFieldNames) {
    store.hset("hash", "a", make_string("1"));
    store.hset("hash", "b", make_string("2"));
    auto keys = store.hkeys("hash");
    EXPECT_EQ(keys.size(), 2u);
}

TEST_F(StoreTest, HvalsReturnsAllValues) {
    store.hset("hash", "f", make_string("v"));
    auto vals = store.hvals("hash");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0].value, "v");
}

TEST_F(StoreTest, HgetallReturnsAllPairs) {
    store.hset("hash", "f1", make_string("v1"));
    store.hset("hash", "f2", make_string("v2"));
    auto all = store.hgetall("hash");
    EXPECT_EQ(all.size(), 2u);
    EXPECT_EQ(all["f1"].value, "v1");
    EXPECT_EQ(all["f2"].value, "v2");
}

TEST_F(StoreTest, HsetUpdatesExistingField) {
    store.hset("hash", "f", make_string("old"));
    store.hset("hash", "f", make_string("new"));
    EXPECT_EQ(store.hlen("hash"), 1u);
    auto result = store.hget("hash", "f");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, "new");
}

TEST_F(StoreTest, TtlReturnsMinus2ForMissingKey) {
    EXPECT_EQ(store.ttl("missing"), -2);
}

TEST_F(StoreTest, TtlReturnsMinus1ForKeyWithoutExpiry) {
    store.set("k", make_string("v"));
    EXPECT_EQ(store.ttl("k"), -1);
}

TEST_F(StoreTest, ExpireSetsTtl) {
    store.set("k", make_string("v"));
    store.expire("k", 5000);
    int64_t ttl = store.ttl("k");
    EXPECT_GT(ttl, 0);
    EXPECT_LE(ttl, 5000);
}

TEST_F(StoreTest, PersistRemovesTtl) {
    store.set("k", make_string("v"));
    store.expire("k", 5000);
    store.persist("k");
    EXPECT_EQ(store.ttl("k"), -1);
}

TEST_F(StoreTest, ExpiredKeyNotVisibleOnGet) {
    store.set("k", make_string("v"));
    store.expire("k", 1);
    usleep(5000);
    EXPECT_FALSE(store.get("k").has_value());
}

TEST_F(StoreTest, CleanupExpiredRemovesExpiredKeys) {
    store.set("k", make_string("v"));
    store.expire("k", 1);
    usleep(5000);
    store.cleanup_expired();
    EXPECT_EQ(store.dbsize(), 0u);
}

TEST_F(StoreTest, KeysWithStarMatchesAll) {
    store.set("a", make_string("1"));
    store.set("b", make_string("2"));
    store.set("c", make_string("3"));
    auto result = store.keys("*");
    EXPECT_EQ(result.size(), 3u);
}

TEST_F(StoreTest, KeysWithPrefixPattern) {
    store.set("user:1", make_string("Alice"));
    store.set("user:2", make_string("Bob"));
    store.set("post:1", make_string("Hello"));
    auto result = store.keys("user:*");
    EXPECT_EQ(result.size(), 2u);
}

TEST_F(StoreTest, KeysWithQuestionMarkPattern) {
    store.set("ab", make_string("1"));
    store.set("ac", make_string("2"));
    store.set("abc", make_string("3"));
    auto result = store.keys("a?");
    EXPECT_EQ(result.size(), 2u);
}

TEST_F(StoreTest, KeysExcludesExpiredKeys) {
    store.set("live", make_string("v"));
    store.set("dead", make_string("v"));
    store.expire("dead", 1);
    usleep(5000);
    auto result = store.keys("*");
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], "live");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
