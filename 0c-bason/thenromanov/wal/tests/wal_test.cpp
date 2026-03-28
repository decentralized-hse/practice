#include "codec/record.hpp"

#include "wal/constants.hpp"
#include "wal/reader.hpp"
#include "wal/writer.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace bason_db;

namespace {

    class WalTest: public ::testing::Test {
    protected:
        void SetUp() override {
            const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            const auto test_dir =
                std::string{test_info->test_suite_name()} + "_" + std::string{test_info->name()};
            test_dir_ = get_test_tmp_dir() / test_dir;
            std::filesystem::create_directories(test_dir_);
        }

        void TearDown() override {
            if (std::filesystem::exists(test_dir_)) {
                std::filesystem::remove_all(test_dir_);
            }
        }

        std::filesystem::path test_dir_;
    };

} // namespace

TEST_F(WalTest, WriteAndRead) {
    {
        auto writer = WalWriter::open(test_dir_);
        auto record1 = BasonRecord{
            .type = BasonType::String,
            .key = "key1",
            .value = "value1",
            .children = {},
        };
        auto record2 = BasonRecord{
            .type = BasonType::String,
            .key = "key2",
            .value = "value2",
            .children = {},
        };
        writer.append(record1);
        writer.append(record2);
        writer.checkpoint();
        writer.sync();
    }

    auto reader = WalReader::open(test_dir_);
    reader.recover();
    auto it = reader.scan(0);

    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.record().key, "key1");
    EXPECT_EQ(it.record().value, "value1");
    it.next();

    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.record().key, "key2");
    EXPECT_EQ(it.record().value, "value2");
    it.next();

    EXPECT_FALSE(it.valid());
}

TEST_F(WalTest, RecoverAfterCheckpoint) {
    {
        auto writer = WalWriter::open(test_dir_);
        auto record1 = BasonRecord{
            .type = BasonType::String,
            .key = "key1",
            .value = "value1",
            .children = {},
        };
        auto record2 = BasonRecord{
            .type = BasonType::String,
            .key = "key2",
            .value = "value2",
            .children = {},
        };
        writer.append(record1);
        writer.checkpoint();
        writer.sync();
        writer.append(record2); // not checkpointed
    }

    auto reader = WalReader::open(test_dir_);
    reader.recover();

    auto it = reader.scan(0);
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.record().key, "key1");
    it.next();
    EXPECT_FALSE(it.valid());
}

TEST_F(WalTest, ScanFromOffset) {
    uint64_t mid_offset = 0;
    {
        auto writer = WalWriter::open(test_dir_);
        auto record1 = BasonRecord{
            .type = BasonType::String,
            .key = "key1",
            .value = "value1",
            .children = {},
        };
        auto record2 = BasonRecord{
            .type = BasonType::String,
            .key = "key2",
            .value = "value2",
            .children = {},
        };
        writer.append(record1);
        writer.checkpoint();
        writer.sync();
        mid_offset = writer.current_offset();
        writer.append(record2);
        writer.checkpoint();
        writer.sync();
    }

    auto reader = WalReader::open(test_dir_);
    reader.recover();
    auto it = reader.scan(mid_offset);

    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.record().key, "key2");
    it.next();
    EXPECT_FALSE(it.valid());
}
