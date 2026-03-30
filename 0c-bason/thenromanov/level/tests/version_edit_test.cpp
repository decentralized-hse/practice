#include "codec/record.hpp"

#include "level/version_edit.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace bason_db;

TEST(VersionEditTest, EmptyEditRoundTrip) {
    auto edit = VersionEdit{};
    auto bason = to_bason(edit);

    auto decoded = VersionEdit{};
    from_bason(bason, decoded);

    EXPECT_TRUE(decoded.additions.empty());
    EXPECT_TRUE(decoded.removals.empty());
    EXPECT_EQ(decoded.flush_offset, 0);
}

TEST(VersionEditTest, SingleAdditionRoundTrip) {
    auto edit = VersionEdit{};
    edit.additions.emplace_back(VersionEdit::FileAdd{
        .level = 0,
        .file = "sst/0000000001.sst",
        .file_size = 4096,
        .first_key = "aaa",
        .last_key = "zzz",
        .min_offset = 10,
        .max_offset = 100,
    });
    edit.flush_offset = 100;

    auto bason = to_bason(edit);

    auto decoded = VersionEdit{};
    from_bason(bason, decoded);

    ASSERT_EQ(decoded.additions.size(), 1);
    EXPECT_EQ(decoded.additions[0].level, 0);
    EXPECT_EQ(decoded.additions[0].file, std::filesystem::path("sst/0000000001.sst"));
    EXPECT_EQ(decoded.additions[0].file_size, 4096);
    EXPECT_EQ(decoded.additions[0].first_key, "aaa");
    EXPECT_EQ(decoded.additions[0].last_key, "zzz");
    EXPECT_EQ(decoded.additions[0].min_offset, 10);
    EXPECT_EQ(decoded.additions[0].max_offset, 100);
    EXPECT_EQ(decoded.flush_offset, 100);
}

TEST(VersionEditTest, SingleRemovalRoundTrip) {
    auto edit = VersionEdit{};
    edit.removals.emplace_back(VersionEdit::FileRemove{
        .level = 1,
        .file = "sst/0000000002.sst",
    });

    auto bason = to_bason(edit);

    auto decoded = VersionEdit{};
    from_bason(bason, decoded);

    ASSERT_EQ(decoded.removals.size(), 1);
    EXPECT_EQ(decoded.removals[0].level, 1);
    EXPECT_EQ(decoded.removals[0].file, std::filesystem::path("sst/0000000002.sst"));
}

TEST(VersionEditTest, ComplexEditRoundTrip) {
    auto edit = VersionEdit{};

    edit.additions.emplace_back(VersionEdit::FileAdd{
        .level = 0,
        .file = "sst/0000000001.sst",
        .file_size = 1024,
        .first_key = "a",
        .last_key = "m",
        .min_offset = 0,
        .max_offset = 50,
    });
    edit.additions.emplace_back(VersionEdit::FileAdd{
        .level = 1,
        .file = "sst/0000000002.sst",
        .file_size = 2048,
        .first_key = "n",
        .last_key = "z",
        .min_offset = 51,
        .max_offset = 100,
    });

    edit.removals.emplace_back(VersionEdit::FileRemove{
        .level = 0,
        .file = "sst/0000000003.sst",
    });
    edit.removals.emplace_back(VersionEdit::FileRemove{
        .level = 0,
        .file = "sst/0000000004.sst",
    });

    edit.flush_offset = 200;

    auto bason = to_bason(edit);

    auto decoded = VersionEdit{};
    from_bason(bason, decoded);

    ASSERT_EQ(decoded.additions.size(), 2);
    ASSERT_EQ(decoded.removals.size(), 2);
    EXPECT_EQ(decoded.flush_offset, 200);

    EXPECT_EQ(decoded.additions[0].first_key, "a");
    EXPECT_EQ(decoded.additions[1].first_key, "n");
    EXPECT_EQ(decoded.removals[0].file, std::filesystem::path("sst/0000000003.sst"));
    EXPECT_EQ(decoded.removals[1].file, std::filesystem::path("sst/0000000004.sst"));
}

TEST(VersionEditTest, ZeroOffsets) {
    auto edit = VersionEdit{};
    edit.additions.emplace_back(VersionEdit::FileAdd{
        .level = 0,
        .file = "sst/test.sst",
        .file_size = 100,
        .first_key = "key",
        .last_key = "key",
        .min_offset = 0,
        .max_offset = 0,
    });
    edit.flush_offset = 0;

    auto bason = to_bason(edit);

    auto decoded = VersionEdit{};
    from_bason(bason, decoded);

    ASSERT_EQ(decoded.additions.size(), 1);
    EXPECT_EQ(decoded.additions[0].min_offset, 0);
    EXPECT_EQ(decoded.additions[0].max_offset, 0);
    EXPECT_EQ(decoded.flush_offset, 0);
}

TEST(VersionEditTest, LargeOffsets) {
    auto edit = VersionEdit{};
    edit.additions.emplace_back(VersionEdit::FileAdd{
        .level = 6,
        .file = "sst/large.sst",
        .file_size = 1ULL << 30, // 1 GB
        .first_key = "start",
        .last_key = "end",
        .min_offset = 1000000,
        .max_offset = 9999999,
    });
    edit.flush_offset = 9999999;

    auto bason = to_bason(edit);

    auto decoded = VersionEdit{};
    from_bason(bason, decoded);

    ASSERT_EQ(decoded.additions.size(), 1);
    EXPECT_EQ(decoded.additions[0].level, 6);
    EXPECT_EQ(decoded.additions[0].file_size, 1ULL << 30);
    EXPECT_EQ(decoded.additions[0].min_offset, 1000000);
    EXPECT_EQ(decoded.additions[0].max_offset, 9999999);
}
