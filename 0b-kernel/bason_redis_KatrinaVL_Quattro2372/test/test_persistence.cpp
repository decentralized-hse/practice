#include <gtest/gtest.h>
#include "../src/redis/persistence_manager.hpp"
#include "../src/redis/bason_redis_store.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace bason;
namespace fs = std::filesystem;

class PersistenceTest : public ::testing::Test {
protected:
    std::string test_dir;

    void SetUp() override {
        test_dir = "/tmp/basonredis_persist_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        fs::remove_all(test_dir);
    }

    BasonRecord make_cmd(std::initializer_list<std::string> args) {
        BasonRecord cmd(BasonType::Array);
        for (const auto& a : args) {
            BasonRecord arg(BasonType::String);
            arg.value = a;
            cmd.children.push_back(arg);
        }
        return cmd;
    }
};

TEST_F(PersistenceTest, ConstructionCreatesDataDir) {
    PersistenceManager pm(test_dir);
    EXPECT_TRUE(fs::exists(test_dir));
}

TEST_F(PersistenceTest, AofFileIsCreatedOnConstruction) {
    PersistenceManager pm(test_dir);
    EXPECT_TRUE(fs::exists(test_dir + "/appendonly.aof"));
}

TEST_F(PersistenceTest, ConstructionDoesNotThrow) {
    EXPECT_NO_THROW(PersistenceManager pm(test_dir));
}

TEST_F(PersistenceTest, LogCommandThrowsWhenCodecIsStub) {
    PersistenceManager pm(test_dir);
    EXPECT_THROW(pm.log_command(make_cmd({"SET", "key", "val"})),
                 std::runtime_error);
}

TEST_F(PersistenceTest, SnapshotThrowsWhenCodecIsStub) {
    BasonRedisStore store;
    PersistenceManager pm(test_dir);
    EXPECT_THROW(pm.snapshot(store, ""), std::runtime_error);
}

TEST_F(PersistenceTest, SnapshotToCustomPathThrowsWhenCodecIsStub) {
    BasonRedisStore store;
    PersistenceManager pm(test_dir);
    EXPECT_THROW(pm.snapshot(store, test_dir + "/custom.bason"),
                 std::runtime_error);
}

TEST_F(PersistenceTest, SyncDoesNotThrow) {
    PersistenceManager pm(test_dir);
    EXPECT_NO_THROW(pm.sync());
}

TEST_F(PersistenceTest, RecoverOnEmptyDirDoesNotThrow) {
    BasonRedisStore store;
    PersistenceManager pm(test_dir);
    EXPECT_NO_THROW(pm.recover(store));
}

TEST_F(PersistenceTest, RecoverWithNonEmptyAofDoesNotThrow) {
    std::string aof_path = test_dir + "/appendonly.aof";
    {
        std::ofstream f(aof_path, std::ios::binary);
        const char data[] = {0x01, 0x02, 0x03, 0x04};
        f.write(data, sizeof(data));
    }

    BasonRedisStore store;
    PersistenceManager pm(test_dir);
    EXPECT_NO_THROW(pm.recover(store));
    EXPECT_EQ(store.dbsize(), 0u);
}

TEST_F(PersistenceTest, RecoverWithNonEmptySnapshotDoesNotThrow) {
    std::string snap_path = test_dir + "/dump.bason";
    {
        std::ofstream f(snap_path, std::ios::binary);
        const char data[] = {0x01, 0x02, 0x03, 0x04};
        f.write(data, sizeof(data));
    }

    BasonRedisStore store;
    PersistenceManager pm(test_dir);
    EXPECT_NO_THROW(pm.recover(store));
    EXPECT_EQ(store.dbsize(), 0u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
