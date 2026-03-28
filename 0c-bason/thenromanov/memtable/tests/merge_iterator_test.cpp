#include "codec/record.hpp"

#include "memtable/memtable.hpp"
#include "memtable/merge_iterator.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace bason_db;

TEST(MergeIteratorTest, SingleSource) {
    auto memtable = Memtable{};
    memtable.put("a", make_string_record("a", "1"), 1);
    memtable.put("b", make_string_record("b", "2"), 2);

    auto sources = std::vector<std::unique_ptr<SortedIterator>>{};
    sources.emplace_back(memtable.scan());

    auto merger = MergeIterator{std::move(sources)};
    ASSERT_TRUE(merger.valid());
    EXPECT_EQ(merger.key(), "a");
    merger.next();
    EXPECT_EQ(merger.key(), "b");
    merger.next();
    EXPECT_FALSE(merger.valid());
}

TEST(MergeIteratorTest, MultipleSources) {
    auto memtable1 = Memtable{};
    auto memtable2 = Memtable{};
    memtable1.put("a", make_string_record("a", "1"), 1);
    memtable1.put("c", make_string_record("c", "3"), 3);
    memtable2.put("b", make_string_record("b", "2"), 2);
    memtable2.put("d", make_string_record("d", "4"), 4);

    auto sources = std::vector<std::unique_ptr<SortedIterator>>{};
    sources.emplace_back(memtable1.scan());
    sources.emplace_back(memtable2.scan());

    auto merger = MergeIterator{std::move(sources)};
    auto keys = std::vector<std::string>{};
    while (merger.valid()) {
        keys.emplace_back(merger.key());
        merger.next();
    }
    EXPECT_EQ(keys, (std::vector<std::string>{"a", "b", "c", "d"}));
}

TEST(MergeIteratorTest, DuplicateKeysHighestOffsetWins) {
    auto memtable1 = Memtable{};
    auto memtable2 = Memtable{};
    memtable1.put("key", make_string_record("key", "old"), 1);
    memtable2.put("key", make_string_record("key", "new"), 10);

    auto sources = std::vector<std::unique_ptr<SortedIterator>>{};
    sources.emplace_back(memtable1.scan());
    sources.emplace_back(memtable2.scan());

    auto merger = MergeIterator{std::move(sources)};
    ASSERT_TRUE(merger.valid());
    EXPECT_EQ(merger.key(), "key");
    EXPECT_EQ(merger.record().value, "new");
    EXPECT_EQ(merger.offset(), 10u);
    merger.next();
    EXPECT_FALSE(merger.valid());
}

TEST(MergeIteratorTest, TombstoneSuppressedBelowMinLiveOffset) {
    auto memtable = Memtable{};
    memtable.put("key", make_tombstone("key"), 5);

    auto sources = std::vector<std::unique_ptr<SortedIterator>>{};
    sources.emplace_back(memtable.scan());

    // min_live_offset = 10, tombstone at offset 5 should be suppressed
    auto merger = MergeIterator{std::move(sources), 10};
    EXPECT_FALSE(merger.valid());
}

TEST(MergeIteratorTest, TombstoneVisibleAboveMinLiveOffset) {
    auto memtable = Memtable{};
    memtable.put("key", make_tombstone("key"), 15);

    auto sources = std::vector<std::unique_ptr<SortedIterator>>{};
    sources.emplace_back(memtable.scan());

    auto merger = MergeIterator{std::move(sources), 10};
    ASSERT_TRUE(merger.valid());
    EXPECT_EQ(merger.key(), "key");
    EXPECT_TRUE(merger.record().value.empty());
}
