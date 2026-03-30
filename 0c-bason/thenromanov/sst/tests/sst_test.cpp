#include "codec/record.hpp"

#include "sst/constants.hpp"
#include "sst/reader.hpp"
#include "sst/writer.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace bason_db;

namespace {

    class SstTest: public ::testing::Test {
    protected:
        void SetUp() override {
            const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            const auto test_dir =
                std::string{test_info->test_suite_name()} + "_" + std::string{test_info->name()};
            test_path_ = get_test_tmp_dir() / test_dir / "file.sst";
            std::filesystem::create_directories(test_path_.parent_path());
        }

        void TearDown() override {
            if (std::filesystem::exists(test_path_.parent_path())) {
                std::filesystem::remove_all(test_path_.parent_path());
            }
        }

        std::filesystem::path test_path_;
    };

} // namespace

TEST_F(SstTest, WriteAndRead) {
    {
        auto writer = SstWriter::open(test_path_);
        writer.add("a", make_string_record("a", "1"));
        writer.add("b", make_string_record("b", "2"));
        writer.add("c", make_string_record("c", "3"));
        auto meta = writer.finish();
        EXPECT_EQ(meta.record_count, 3U);
        EXPECT_EQ(meta.first_key, "a");
        EXPECT_EQ(meta.last_key, "c");
    }

    auto reader = SstReader::open(test_path_);
    auto meta = reader.metadata();
    EXPECT_EQ(meta.record_count, 3U);

    auto record = reader.get("b");
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->first.value, "2");

    auto missing = reader.get("durian");
    EXPECT_FALSE(missing.has_value());
}

TEST_F(SstTest, RangeScan) {
    {
        auto writer = SstWriter::open(test_path_);
        writer.add("a", make_string_record("a", "1"));
        writer.add("b", make_string_record("b", "2"));
        writer.add("c", make_string_record("c", "3"));
        writer.add("d", make_string_record("d", "4"));
        writer.add("e", make_string_record("e", "5"));
        writer.finish();
    }

    auto reader = SstReader::open(test_path_);
    auto it = reader.scan("b", "d");

    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "b");
    it.next();
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.key(), "c");
    it.next();
    EXPECT_FALSE(it.valid()); // "d" is exclusive
}

TEST_F(SstTest, FullScan) {
    std::vector<std::string> keys = {
        "a", "b", "g", "d", "e",
    };
    std::sort(keys.begin(), keys.end());
    {
        auto writer = SstWriter::open(test_path_);
        for (const auto& k : keys) {
            writer.add(k, make_string_record(k, k + "_val"));
        }
        writer.finish();
    }

    auto reader = SstReader::open(test_path_);
    auto it = reader.scan();
    size_t count = 0;
    while (it.valid()) {
        EXPECT_EQ(it.key(), keys[count]);
        ++count;
        it.next();
    }
    EXPECT_EQ(count, keys.size());
}

TEST_F(SstTest, OutOfOrderThrows) {
    auto writer = SstWriter::open(test_path_);
    writer.add("b", make_string_record("b", "2"));
    EXPECT_THROW(writer.add("a", make_string_record("a", "1")), std::runtime_error);
}

TEST_F(SstTest, LargeFile) {
    constexpr int kTotalKeys = 1000;
    {
        auto writer = SstWriter::open(test_path_);
        for (int i = 0; i < kTotalKeys; ++i) {
            auto key = std::string(6 - std::to_string(i).size(), '0') + std::to_string(i);
            writer.add(key, make_string_record(key, "value_" + std::to_string(i)));
        }
        writer.finish();
    }

    auto reader = SstReader::open(test_path_);
    for (int i = 0; i < kTotalKeys; ++i) {
        auto key = std::string(6 - std::to_string(i).size(), '0') + std::to_string(i);
        auto record = reader.get(key);
        ASSERT_TRUE(record.has_value());
        EXPECT_EQ(record->first.value, "value_" + std::to_string(i));
    }
}
