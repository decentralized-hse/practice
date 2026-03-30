#include "wal/iterator.hpp"

#include <gtest/gtest.h>

using namespace bason_db;

TEST(WalIteratorTest, EmptyIteratorIsInvalid) {
    auto empty_entries = std::vector<WalIterator::Entry>{};

    auto it = WalIterator{std::move(empty_entries)};

    EXPECT_FALSE(it.valid());
}

TEST(WalIteratorTest, IteratesSuccessfully) {
    auto entries = std::vector<WalIterator::Entry>{};

    entries.emplace_back(WalIterator::Entry{
        .offset = 10,
        .record = BasonRecord{},
    });
    entries.emplace_back(WalIterator::Entry{
        .offset = 20,
        .record = BasonRecord{},
    });
    entries.emplace_back(WalIterator::Entry{
        .offset = 30,
        .record = BasonRecord{},
    });

    auto it = WalIterator{std::move(entries)};

    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.offset(), 10);

    it.next();
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.offset(), 20);

    it.next();
    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.offset(), 30);

    it.next();
    EXPECT_FALSE(it.valid());
}

TEST(WalIteratorTest, StartIndexParameter) {
    auto entries = std::vector<WalIterator::Entry>{};

    entries.emplace_back(WalIterator::Entry{
        .offset = 10,
        .record = BasonRecord{},
    });
    entries.emplace_back(WalIterator::Entry{
        .offset = 20,
        .record = BasonRecord{},
    });

    auto it = WalIterator{std::move(entries), 1};

    ASSERT_TRUE(it.valid());
    EXPECT_EQ(it.offset(), 20);
    it.next();
    EXPECT_FALSE(it.valid());
}
