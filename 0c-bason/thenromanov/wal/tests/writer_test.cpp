#include "codec/record.hpp"

#include "wal/constants.hpp"
#include "wal/writer.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <vector>

using namespace bason_db;

namespace {

    class WalWriterTest: public ::testing::Test {
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

        int count_segments() const {
            int count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(test_dir_)) {
                if (entry.path().extension() == kWalExtension) {
                    ++count;
                }
            }
            return count;
        }

        std::filesystem::path test_dir_;
    };

} // namespace

TEST_F(WalWriterTest, OpenCreatesDirectoryAndInitialSegment) {
    auto writer = WalWriter::open(test_dir_);

    EXPECT_TRUE(std::filesystem::exists(test_dir_));
    EXPECT_EQ(count_segments(), 1);

    EXPECT_EQ(writer.current_offset(), kHeaderSize);
}

TEST_F(WalWriterTest, AppendIncrementsOffsetAndWritesData) {
    auto writer = WalWriter::open(test_dir_);
    uint64_t initial_offset = writer.current_offset();

    auto record = BasonRecord{
        .type = BasonType::String,
        .key = "key",
        .value = "value",
        .children = {},
    };

    uint64_t returned_offset = writer.append(record);

    EXPECT_EQ(returned_offset, initial_offset);

    EXPECT_GT(writer.current_offset(), initial_offset);
    EXPECT_EQ(writer.current_offset(), initial_offset + 16);
}

TEST_F(WalWriterTest, CheckpointIncreasesOffsetCorrectly) {
    auto writer = WalWriter::open(test_dir_);
    uint64_t before_checkpoint = writer.current_offset();

    writer.checkpoint();

    EXPECT_EQ(writer.current_offset(), before_checkpoint + 5);
}

TEST_F(WalWriterTest, SyncDoesNotCrash) {
    auto writer = WalWriter::open(test_dir_);

    auto record = BasonRecord{
        .type = BasonType::String,
        .key = "key",
        .value = "value",
        .children = {},
    };
    writer.append(record);

    EXPECT_NO_THROW(writer.sync());
}

TEST_F(WalWriterTest, RotateCreatesNewSegments) {
    auto writer = WalWriter::open(test_dir_.string());

    auto record = BasonRecord{
        .type = BasonType::String,
        .key = "key",
        .value = "value",
        .children = {},
    };
    writer.append(record);

    uint64_t size_after_one = writer.current_offset();

    writer.rotate(1);

    EXPECT_EQ(count_segments(), 2);

    EXPECT_EQ(writer.current_offset(), size_after_one + kHeaderSize);
}

TEST_F(WalWriterTest, ResumeRecoversDirectoryAndOffsets) {
    uint64_t final_offset_before_close = 0;

    {
        auto writer = WalWriter::open(test_dir_);

        auto record = BasonRecord{
            .type = BasonType::String,
            .key = "key",
            .value = "value",
            .children = {},
        };
        writer.append(record);
        writer.append(record);

        writer.checkpoint();
        writer.sync();

        final_offset_before_close = writer.current_offset();
    }

    {
        auto writer = WalWriter::open(test_dir_);

        EXPECT_EQ(writer.current_offset(), final_offset_before_close);
        EXPECT_EQ(count_segments(), 1);
    }
}
