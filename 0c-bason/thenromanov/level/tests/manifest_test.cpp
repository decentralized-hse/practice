#include "level/manifest.hpp"

#include "wal/reader.hpp"
#include "wal/writer.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using namespace bason_db;

namespace {

    class ManifestTest: public ::testing::Test {
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

        std::filesystem::path test_dir_;
    };

} // namespace

TEST_F(ManifestTest, LogAndReplay) {
    {
        auto manifest = Manifest::open(test_dir_);

        auto edit = VersionEdit{};
        edit.additions.emplace_back(VersionEdit::FileAdd{
            .level = 0,
            .file = "sst/0000000001.sst",
            .file_size = 4096,
            .first_key = "aaa",
            .last_key = "zzz",
            .min_offset = 0,
            .max_offset = 100,
        });
        edit.flush_offset = 100;
        manifest.log_edit(edit);
    }

    {
        auto manifest = Manifest::open(test_dir_);
        auto edits = manifest.replay();
        ASSERT_EQ(edits.size(), 1U);
        EXPECT_EQ(edits[0].additions.size(), 1U);
        EXPECT_EQ(edits[0].additions[0].level, 0);
        EXPECT_EQ(edits[0].additions[0].first_key, "aaa");
        EXPECT_EQ(edits[0].additions[0].last_key, "zzz");
        EXPECT_EQ(edits[0].additions[0].file_size, 4096u);
        EXPECT_EQ(edits[0].flush_offset, 100u);
    }
}

TEST_F(ManifestTest, MultipleEdits) {
    {
        auto manifest = Manifest::open(test_dir_);

        auto edit1 = VersionEdit{};
        edit1.additions.emplace_back(VersionEdit::FileAdd{
            .level = 0,
            .file = "sst/0000000001.sst",
            .file_size = 1024,
            .first_key = "a",
            .last_key = "m",
        });
        edit1.flush_offset = 50;
        manifest.log_edit(edit1);

        auto edit2 = VersionEdit{};
        edit2.additions.emplace_back(VersionEdit::FileAdd{
            .level = 1,
            .file = "sst/0000000002.sst",
            .file_size = 2048,
            .first_key = "a",
            .last_key = "z",
        });
        edit2.removals.emplace_back(VersionEdit::FileRemove{
            .level = 0,
            .file = "sst/0000000001.sst",
        });
        manifest.log_edit(edit2);
    }

    {
        auto manifest = Manifest::open(test_dir_);
        auto edits = manifest.replay();
        ASSERT_EQ(edits.size(), 2U);

        EXPECT_EQ(edits[0].additions.size(), 1U);
        EXPECT_EQ(edits[0].removals.size(), 0U);

        EXPECT_EQ(edits[1].additions.size(), 1U);
        EXPECT_EQ(edits[1].removals.size(), 1U);
        EXPECT_EQ(edits[1].removals[0].file, std::filesystem::path("sst/0000000001.sst"));
    }
}

TEST_F(ManifestTest, EmptyReplay) {
    auto manifest = Manifest::open(test_dir_);
    auto edits = manifest.replay();
    EXPECT_TRUE(edits.empty());
}
