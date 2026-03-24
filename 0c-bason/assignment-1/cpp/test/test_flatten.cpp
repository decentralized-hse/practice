#include "../src/flatten.h"

#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

TEST(FlattenTest, SingleLeaf)
{
    TBasonRecord record(EBasonType::String, "name", "Alice");
    auto flat = FlattenBason(record);
    
    ASSERT_EQ(flat.size(), 1);
    EXPECT_EQ(flat[0].Key, "name");
    EXPECT_EQ(flat[0].Value, "Alice");
}

TEST(FlattenTest, SimpleObject)
{
    TBasonRecord object(EBasonType::Object, "", "");
    object.Children.push_back(TBasonRecord(EBasonType::String, "name", "Alice"));
    object.Children.push_back(TBasonRecord(EBasonType::Number, "age", "25"));
    
    auto flat = FlattenBason(object);
    
    ASSERT_EQ(flat.size(), 2);
    EXPECT_EQ(flat[0].Key, "name");
    EXPECT_EQ(flat[0].Value, "Alice");
    EXPECT_EQ(flat[1].Key, "age");
    EXPECT_EQ(flat[1].Value, "25");
}

TEST(FlattenTest, NestedObject)
{
    TBasonRecord root(EBasonType::Object, "", "");
    
    TBasonRecord user(EBasonType::Object, "user", "");
    user.Children.push_back(TBasonRecord(EBasonType::String, "name", "Alice"));
    user.Children.push_back(TBasonRecord(EBasonType::Number, "age", "25"));
    
    root.Children.push_back(user);
    
    auto flat = FlattenBason(root);
    
    ASSERT_EQ(flat.size(), 2);
    EXPECT_EQ(flat[0].Key, "user/name");
    EXPECT_EQ(flat[0].Value, "Alice");
    EXPECT_EQ(flat[1].Key, "user/age");
    EXPECT_EQ(flat[1].Value, "25");
}

TEST(FlattenTest, Array)
{
    TBasonRecord array(EBasonType::Array, "scores", "");
    array.Children.push_back(TBasonRecord(EBasonType::Number, "0", "95"));
    array.Children.push_back(TBasonRecord(EBasonType::Number, "1", "87"));
    
    auto flat = FlattenBason(array);
    
    ASSERT_EQ(flat.size(), 2);
    EXPECT_EQ(flat[0].Key, "scores/0");
    EXPECT_EQ(flat[1].Key, "scores/1");
}

TEST(UnflattenTest, SingleLeaf)
{
    std::vector<TBasonRecord> flat;
    flat.push_back(TBasonRecord(EBasonType::String, "name", "Alice"));
    
    auto nested = UnflattenBason(flat);
    
    ASSERT_EQ(nested.Children.size(), 1);
    EXPECT_EQ(nested.Children[0].Key, "name");
    EXPECT_EQ(nested.Children[0].Value, "Alice");
}

TEST(UnflattenTest, MultipleLeaves)
{
    std::vector<TBasonRecord> flat;
    flat.push_back(TBasonRecord(EBasonType::String, "name", "Alice"));
    flat.push_back(TBasonRecord(EBasonType::Number, "age", "25"));
    
    auto nested = UnflattenBason(flat);
    
    ASSERT_EQ(nested.Children.size(), 2);
    // std::map сортирует ключи лексикографически: "age" < "name"
    EXPECT_EQ(nested.Children[0].Key, "age");
    EXPECT_EQ(nested.Children[1].Key, "name");
}

TEST(UnflattenTest, NestedPaths)
{
    std::vector<TBasonRecord> flat;
    flat.push_back(TBasonRecord(EBasonType::String, "user/name", "Alice"));
    flat.push_back(TBasonRecord(EBasonType::Number, "user/age", "25"));
    
    auto nested = UnflattenBason(flat);
    
    ASSERT_EQ(nested.Children.size(), 1);
    EXPECT_EQ(nested.Children[0].Key, "user");
    ASSERT_EQ(nested.Children[0].Children.size(), 2);
}

TEST(UnflattenTest, ArrayIndices)
{
    std::vector<TBasonRecord> flat;
    flat.push_back(TBasonRecord(EBasonType::Number, "scores/0", "95"));
    flat.push_back(TBasonRecord(EBasonType::Number, "scores/1", "87"));
    
    auto nested = UnflattenBason(flat);
    
    ASSERT_EQ(nested.Children.size(), 1);
    EXPECT_EQ(nested.Children[0].Key, "scores");
    EXPECT_EQ(nested.Children[0].Type, EBasonType::Array);
    ASSERT_EQ(nested.Children[0].Children.size(), 2);
}

TEST(FlattenUnflattenTest, RoundTrip)
{
    TBasonRecord original(EBasonType::Object, "", "");
    
    TBasonRecord user(EBasonType::Object, "user", "");
    user.Children.push_back(TBasonRecord(EBasonType::String, "name", "Alice"));
    user.Children.push_back(TBasonRecord(EBasonType::Number, "age", "25"));
    
    original.Children.push_back(user);
    
    auto flat = FlattenBason(original);
    auto nested = UnflattenBason(flat);
    
    // Check structure is preserved
    ASSERT_EQ(nested.Children.size(), 1);
    EXPECT_EQ(nested.Children[0].Key, "user");
    ASSERT_EQ(nested.Children[0].Children.size(), 2);
}

////////////////////////////////////////////////////////////////////////////////
