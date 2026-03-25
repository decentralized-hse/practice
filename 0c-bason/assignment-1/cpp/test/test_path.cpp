#include "../src/path_utils.h"

#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

TEST(PathUtilsTest, JoinEmpty)
{
    EXPECT_EQ(JoinPath({}), "");
}

TEST(PathUtilsTest, JoinSingle)
{
    EXPECT_EQ(JoinPath({"foo"}), "foo");
}

TEST(PathUtilsTest, JoinMultiple)
{
    EXPECT_EQ(JoinPath({"a", "b", "c"}), "a/b/c");
    EXPECT_EQ(JoinPath({"users", "0", "name"}), "users/0/name");
}

TEST(PathUtilsTest, SplitEmpty)
{
    auto result = SplitPath("");
    EXPECT_TRUE(result.empty());
}

TEST(PathUtilsTest, SplitSingle)
{
    auto result = SplitPath("foo");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "foo");
}

TEST(PathUtilsTest, SplitMultiple)
{
    auto result = SplitPath("a/b/c");
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
}

TEST(PathUtilsTest, SplitWithLeadingSlash)
{
    auto result = SplitPath("/a/b");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
}

TEST(PathUtilsTest, SplitWithTrailingSlash)
{
    auto result = SplitPath("a/b/");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
}

TEST(PathUtilsTest, GetPathParentEmpty)
{
    EXPECT_EQ(GetPathParent(""), "");
}

TEST(PathUtilsTest, GetPathParentNoSlash)
{
    EXPECT_EQ(GetPathParent("foo"), "");
}

TEST(PathUtilsTest, GetPathParentWithSlash)
{
    EXPECT_EQ(GetPathParent("a/b/c"), "a/b");
    EXPECT_EQ(GetPathParent("a/b"), "a");
}

TEST(PathUtilsTest, GetPathBasenameEmpty)
{
    EXPECT_EQ(GetPathBasename(""), "");
}

TEST(PathUtilsTest, GetPathBasenameNoSlash)
{
    EXPECT_EQ(GetPathBasename("foo"), "foo");
}

TEST(PathUtilsTest, GetPathBasenameWithSlash)
{
    EXPECT_EQ(GetPathBasename("a/b/c"), "c");
    EXPECT_EQ(GetPathBasename("users/0/name"), "name");
}

TEST(PathUtilsTest, RoundTrip)
{
    std::vector<std::string> segments = {"a", "b", "c", "d"};
    std::string joined = JoinPath(segments);
    auto split = SplitPath(joined);
    EXPECT_EQ(split, segments);
}

////////////////////////////////////////////////////////////////////////////////
