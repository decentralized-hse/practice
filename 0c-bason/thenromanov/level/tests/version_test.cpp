#include "codec/record.hpp"

#include "level/version.hpp"

#include "sst/reader.hpp"
#include "sst/writer.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using namespace bason_db;

namespace {

    class VersionTest: public ::testing::Test {
    protected:
        void SetUp() override {
            const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            auto test_dir =
                std::string{test_info->test_suite_name()} + "_" + std::string{test_info->name()};
            test_dir_ = get_test_tmp_dir() / test_dir;
            std::filesystem::create_directories(test_dir_);
        }

        void TearDown() override {
            if (std::filesystem::exists(test_dir_)) {
                std::filesystem::remove_all(test_dir_);
            }
        }

        std::filesystem::path
        make_sst(const std::string& name,
                 const std::vector<std::pair<std::string, std::string>>& kvs) {
            auto path = test_dir_ / name;
            auto writer = SstWriter::open(path);
            for (const auto& [k, v] : kvs) {
                writer.add(k, make_string_record(k, v));
            }
            writer.finish();
            return path;
        }

        std::filesystem::path test_dir_;
    };

} // namespace

TEST_F(VersionTest, AddAndRemoveFile) {
    auto path = make_sst("test.sst", {{"a", "1"}, {"b", "2"}, {"c", "3"}});

    auto version = Version{};
    auto meta = SstMetadata{};
    meta.path = path;
    meta.first_key = "a";
    meta.last_key = "c";
    meta.file_size = std::filesystem::file_size(path);

    version.add_file(0, meta);
    EXPECT_EQ(version.num_files(0), 1U);
    EXPECT_GT(version.total_bytes(0), 0U);

    version.remove_file(0, path);
    EXPECT_EQ(version.num_files(0), 0U);
}

TEST_F(VersionTest, OverlappingFiles) {
    auto path1 = make_sst("test1.sst", {{"a", "1"}, {"c", "3"}});
    auto path2 = make_sst("test2.sst", {{"d", "4"}, {"f", "6"}});
    auto path3 = make_sst("test3.sst", {{"g", "7"}, {"i", "9"}});

    auto version = Version{};

    auto meta1 = SstMetadata{
        .path = path1,
        .first_key = "a",
        .last_key = "c",
    };
    auto meta2 = SstMetadata{
        .path = path2,
        .first_key = "d",
        .last_key = "f",
    };
    auto meta3 = SstMetadata{
        .path = path3,
        .first_key = "g",
        .last_key = "i",
    };

    version.add_file(1, meta1);
    version.add_file(1, meta2);
    version.add_file(1, meta3);

    auto overlap = version.overlapping_files(1, "b", "e");
    EXPECT_EQ(overlap.size(), 2U);

    auto no_overlap = version.overlapping_files(1, "j", "z");
    EXPECT_EQ(no_overlap.size(), 0U);
}

TEST_F(VersionTest, SortedFiles) {
    auto path1 = make_sst("test1.sst", {{"d", "4"}, {"f", "6"}});
    auto path2 = make_sst("test2.sst", {{"a", "1"}, {"c", "3"}});

    auto version = Version{};

    auto meta1 = SstMetadata{
        .path = path1,
        .first_key = "d",
        .last_key = "f",
    };
    auto meta2 = SstMetadata{
        .path = path2,
        .first_key = "a",
        .last_key = "c",
    };

    version.add_file(1, meta1);
    version.add_file(1, meta2);

    auto sorted = version.sorted_files(1);
    ASSERT_EQ(sorted.size(), 2U);
    EXPECT_EQ(sorted[0].meta.first_key, "a");
    EXPECT_EQ(sorted[1].meta.first_key, "d");
}
