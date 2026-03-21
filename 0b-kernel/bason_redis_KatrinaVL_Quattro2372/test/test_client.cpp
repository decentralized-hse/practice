#include <gtest/gtest.h>
#include "../src/redis/bason_redis_client.hpp"

using namespace bason;

TEST(ClientConnectionTest, DefaultConstructedClientIsNotConnected) {
    BasonRedisClient client;
    EXPECT_NO_THROW(client.disconnect());
}

TEST(ClientConnectionTest, ConnectToNonListeningPortThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.connect("127.0.0.1", 1), std::runtime_error);
}

TEST(ClientConnectionTest, DisconnectAfterFailedConnectDoesNotThrow) {
    BasonRedisClient client;
    try { client.connect("127.0.0.1", 1); } catch (...) {}
    EXPECT_NO_THROW(client.disconnect());
}

TEST(ClientConnectionTest, DoubleDisconnectDoesNotThrow) {
    BasonRedisClient client;
    client.disconnect();
    EXPECT_NO_THROW(client.disconnect());
}

TEST(ClientCommandTest, ExecuteOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.execute({"PING"}), std::runtime_error);
}

TEST(ClientCommandTest, SetOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.set("k", "v"), std::runtime_error);
}

TEST(ClientCommandTest, GetOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.get("k"), std::runtime_error);
}

TEST(ClientCommandTest, DelOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.del("k"), std::runtime_error);
}

TEST(ClientCommandTest, ExistsOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.exists("k"), std::runtime_error);
}

TEST(ClientCommandTest, IncrOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.incr("k"), std::runtime_error);
}

TEST(ClientCommandTest, IncrbyOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.incrby("k", 5), std::runtime_error);
}

TEST(ClientCommandTest, LpushOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.lpush("k", "v"), std::runtime_error);
}

TEST(ClientCommandTest, RpushOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.rpush("k", "v"), std::runtime_error);
}

TEST(ClientCommandTest, LlenOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.llen("k"), std::runtime_error);
}

TEST(ClientCommandTest, HsetOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.hset("k", "f", "v"), std::runtime_error);
}

TEST(ClientCommandTest, HgetOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.hget("k", "f"), std::runtime_error);
}

TEST(ClientCommandTest, ExpireOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.expire("k", 1000), std::runtime_error);
}

TEST(ClientCommandTest, TtlOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.ttl("k"), std::runtime_error);
}

TEST(ClientCommandTest, KeysOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.keys("*"), std::runtime_error);
}

TEST(ClientCommandTest, DbsizeOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.dbsize(), std::runtime_error);
}

TEST(ClientCommandTest, PingOnUnconnectedClientThrows) {
    BasonRedisClient client;
    EXPECT_THROW(client.ping(), std::runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
