#include "level/snapshot.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace bason_db;

TEST(SnapshotListTest, EmptyListIsEmpty) {
    auto list = SnapshotList{};
    EXPECT_TRUE(list.empty());
}

TEST(SnapshotListTest, EmplaceBackAddsSnapshot) {
    auto list = SnapshotList{};
    auto* snapshot = list.emplace_back(42);
    ASSERT_NE(snapshot, nullptr);
    EXPECT_EQ(snapshot->offset, 42);
    EXPECT_FALSE(list.empty());
}

TEST(SnapshotListTest, FrontReturnsOldestSnapshot) {
    auto list = SnapshotList{};
    list.emplace_back(10);
    list.emplace_back(20);
    list.emplace_back(30);

    EXPECT_EQ(list.front()->offset, 10);
}

TEST(SnapshotListTest, EraseRemovesSnapshot) {
    auto list = SnapshotList{};
    auto* s1 = list.emplace_back(10);
    auto* s2 = list.emplace_back(20);
    auto* s3 = list.emplace_back(30);

    list.erase(s2);

    EXPECT_EQ(list.front()->offset, 10);
    EXPECT_FALSE(list.empty());

    list.erase(s1);
    EXPECT_EQ(list.front()->offset, 30);

    list.erase(s3);
    EXPECT_TRUE(list.empty());
}

TEST(SnapshotListTest, EraseFirstUpdatesHead) {
    auto list = SnapshotList{};
    auto* s1 = list.emplace_back(10);
    list.emplace_back(20);

    list.erase(s1);
    EXPECT_EQ(list.front()->offset, 20);
}

TEST(SnapshotListTest, EraseLastLeavesListValid) {
    auto list = SnapshotList{};
    list.emplace_back(10);
    auto* s2 = list.emplace_back(20);

    list.erase(s2);
    EXPECT_EQ(list.front()->offset, 10);
    EXPECT_FALSE(list.empty());
}

TEST(SnapshotListTest, MultipleSnapshotsAtSameOffset) {
    auto list = SnapshotList{};
    auto* s1 = list.emplace_back(42);
    auto* s2 = list.emplace_back(42);
    auto* s3 = list.emplace_back(42);

    EXPECT_EQ(list.front()->offset, 42);

    list.erase(s1);
    EXPECT_EQ(list.front()->offset, 42);

    list.erase(s2);
    EXPECT_EQ(list.front()->offset, 42);

    list.erase(s3);
    EXPECT_TRUE(list.empty());
}

TEST(SnapshotListTest, OrderPreserved) {
    auto list = SnapshotList{};
    list.emplace_back(50);
    list.emplace_back(100);
    list.emplace_back(200);

    EXPECT_EQ(list.front()->offset, 50);
}

TEST(SnapshotListTest, ThrowsOnBadOrder) {
    auto list = SnapshotList{};
    list.emplace_back(100);
    EXPECT_THROW(list.emplace_back(50), std::runtime_error);
}

TEST(SnapshotListTest, LargeNumberOfSnapshots) {
    auto list = SnapshotList{};
    auto snapshots = std::vector<Snapshot*>{};

    constexpr int kCount = 1000;
    for (int i = 0; i < kCount; ++i) {
        snapshots.push_back(list.emplace_back(static_cast<uint64_t>(i)));
    }

    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.front()->offset, 0);

    for (int i = kCount - 1; i >= 0; --i) {
        list.erase(snapshots[static_cast<size_t>(i)]);
    }

    EXPECT_TRUE(list.empty());
}

TEST(SnapshotListTest, EraseMiddlePreservesOrder) {
    auto list = SnapshotList{};
    auto* s1 = list.emplace_back(1);
    auto* s2 = list.emplace_back(2);
    auto* s3 = list.emplace_back(3);
    auto* s4 = list.emplace_back(4);
    auto* s5 = list.emplace_back(5);

    list.erase(s2);
    list.erase(s4);

    EXPECT_EQ(list.front()->offset, 1);

    list.erase(s1);
    EXPECT_EQ(list.front()->offset, 3);

    list.erase(s3);
    EXPECT_EQ(list.front()->offset, 5);

    list.erase(s5);
    EXPECT_TRUE(list.empty());
}
