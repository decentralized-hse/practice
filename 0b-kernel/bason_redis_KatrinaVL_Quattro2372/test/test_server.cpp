#include <gtest/gtest.h>
#include "../src/redis/bason_redis_server.hpp"

using namespace bason;

class ServerTest : public ::testing::Test {
protected:
    BasonRedisServer server{"/tmp/basonredis_test_server"};

    std::string cmd(std::initializer_list<std::string> args) {
        return server.execute_command(
            std::vector<std::string>(args)).value;
    }

    BasonRecord cmd_rec(std::initializer_list<std::string> args) {
        return server.execute_command(std::vector<std::string>(args));
    }
};

TEST_F(ServerTest, PingReturnsPong) {
    EXPECT_EQ(cmd({"PING"}), "PONG");
}

TEST_F(ServerTest, SetReturnsOk) {
    EXPECT_EQ(cmd({"SET", "k", "v"}), "OK");
}

TEST_F(ServerTest, GetReturnsSetValue) {
    cmd({"SET", "hello", "world"});
    EXPECT_EQ(cmd({"GET", "hello"}), "world");
}

TEST_F(ServerTest, GetMissingKeyReturnsBooleanEmpty) {
    auto resp = cmd_rec({"GET", "no_such_key"});
    EXPECT_EQ(resp.type, BasonType::Boolean);
    EXPECT_TRUE(resp.value.empty());
}

TEST_F(ServerTest, DelExistingKeyReturns1) {
    cmd({"SET", "x", "1"});
    EXPECT_EQ(cmd({"DEL", "x"}), "1");
}

TEST_F(ServerTest, DelMissingKeyReturns0) {
    EXPECT_EQ(cmd({"DEL", "missing"}), "0");
}

TEST_F(ServerTest, ExistsAfterSet) {
    cmd({"SET", "e", "v"});
    EXPECT_EQ(cmd({"EXISTS", "e"}), "1");
}

TEST_F(ServerTest, ExistsMissingKey) {
    EXPECT_EQ(cmd({"EXISTS", "no_key"}), "0");
}

TEST_F(ServerTest, IncrCreatesAndReturns1) {
    EXPECT_EQ(cmd({"INCR", "ctr"}), "1");
}

TEST_F(ServerTest, IncrIncrements) {
    cmd({"INCR", "ctr"});
    EXPECT_EQ(cmd({"INCR", "ctr"}), "2");
}

TEST_F(ServerTest, IncrbyAdds) {
    cmd({"INCR", "ctr"});
    EXPECT_EQ(cmd({"INCRBY", "ctr", "9"}), "10");
}

TEST_F(ServerTest, LpushReturnsNewLength) {
    EXPECT_EQ(cmd({"LPUSH", "list", "a"}), "1");
    EXPECT_EQ(cmd({"LPUSH", "list", "b"}), "2");
}

TEST_F(ServerTest, RpushReturnsNewLength) {
    EXPECT_EQ(cmd({"RPUSH", "list", "a"}), "1");
    EXPECT_EQ(cmd({"RPUSH", "list", "b"}), "2");
}

TEST_F(ServerTest, LlenAfterPushes) {
    cmd({"RPUSH", "list", "a"});
    cmd({"RPUSH", "list", "b"});
    EXPECT_EQ(cmd({"LLEN", "list"}), "2");
}

TEST_F(ServerTest, HsetReturns1) {
    EXPECT_EQ(cmd({"HSET", "h", "f", "v"}), "1");
}

TEST_F(ServerTest, HgetReturnsValue) {
    cmd({"HSET", "h", "field", "val"});
    EXPECT_EQ(cmd({"HGET", "h", "field"}), "val");
}

TEST_F(ServerTest, HgetMissingFieldReturnsBooleanEmpty) {
    cmd({"HSET", "h", "f", "v"});
    auto resp = cmd_rec({"HGET", "h", "no_field"});
    EXPECT_EQ(resp.type, BasonType::Boolean);
    EXPECT_TRUE(resp.value.empty());
}

TEST_F(ServerTest, ExpireAndTtlPositive) {
    cmd({"SET", "k", "v"});
    cmd({"EXPIRE", "k", "5000"});
    auto resp = cmd_rec({"TTL", "k"});
    int64_t ttl = std::stoll(resp.value);
    EXPECT_GT(ttl, 0);
    EXPECT_LE(ttl, 5000);
}

TEST_F(ServerTest, TtlMinus1ForNoExpiry) {
    cmd({"SET", "k", "v"});
    EXPECT_EQ(cmd({"TTL", "k"}), "-1");
}

TEST_F(ServerTest, TtlMinus2ForMissingKey) {
    EXPECT_EQ(cmd({"TTL", "no_key"}), "-2");
}

TEST_F(ServerTest, KeysStarReturnsAll) {
    cmd({"SET", "a", "1"});
    cmd({"SET", "b", "2"});
    auto resp = cmd_rec({"KEYS", "*"});
    EXPECT_EQ(resp.type, BasonType::Array);
    EXPECT_EQ(resp.children.size(), 2u);
}

TEST_F(ServerTest, KeysPrefixPattern) {
    cmd({"SET", "user:1", "Alice"});
    cmd({"SET", "user:2", "Bob"});
    cmd({"SET", "post:1", "Hello"});
    auto resp = cmd_rec({"KEYS", "user:*"});
    EXPECT_EQ(resp.children.size(), 2u);
}

TEST_F(ServerTest, DbsizeReflectsCount) {
    cmd({"SET", "a", "1"});
    cmd({"SET", "b", "2"});
    EXPECT_EQ(cmd({"DBSIZE"}), "2");
}

TEST_F(ServerTest, UnknownCommandReturnsError) {
    auto resp = cmd_rec({"FOOBAR"});
    EXPECT_EQ(resp.type, BasonType::String);
    EXPECT_NE(resp.value.find("ERR"), std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
