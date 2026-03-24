#include "../src/bason_codec.h"

#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

TEST(BasonCodecTest, EncodeDecodeString)
{
    TBasonRecord record(EBasonType::String, "name", "Alice");
    auto encoded = EncodeBason(record);
    
    auto [decoded, size] = DecodeBason(encoded.data(), encoded.size());
    
    EXPECT_EQ(decoded.Type, EBasonType::String);
    EXPECT_EQ(decoded.Key, "name");
    EXPECT_EQ(decoded.Value, "Alice");
    EXPECT_EQ(size, encoded.size());
}

TEST(BasonCodecTest, EncodeDecodeNumber)
{
    TBasonRecord record(EBasonType::Number, "age", "25");
    auto encoded = EncodeBason(record);
    
    auto [decoded, size] = DecodeBason(encoded.data(), encoded.size());
    
    EXPECT_EQ(decoded.Type, EBasonType::Number);
    EXPECT_EQ(decoded.Key, "age");
    EXPECT_EQ(decoded.Value, "25");
}

TEST(BasonCodecTest, EncodeDecodeBoolean)
{
    TBasonRecord record(EBasonType::Boolean, "active", "true");
    auto encoded = EncodeBason(record);
    
    auto [decoded, size] = DecodeBason(encoded.data(), encoded.size());
    
    EXPECT_EQ(decoded.Type, EBasonType::Boolean);
    EXPECT_EQ(decoded.Value, "true");
}

TEST(BasonCodecTest, EncodeDecodeBooleanNull)
{
    TBasonRecord record(EBasonType::Boolean, "value", "");
    auto encoded = EncodeBason(record);
    
    auto [decoded, size] = DecodeBason(encoded.data(), encoded.size());
    
    EXPECT_EQ(decoded.Type, EBasonType::Boolean);
    EXPECT_EQ(decoded.Value, "");  // null
}

TEST(BasonCodecTest, ShortFormUsed)
{
    // Key: 4 bytes, Value: 5 bytes - should use short form
    TBasonRecord record(EBasonType::String, "name", "Alice");
    auto encoded = EncodeBason(record);
    
    // Tag + lengths (2 bytes) + key (4) + value (5) = 11 bytes
    EXPECT_EQ(encoded.size(), 11);
    EXPECT_EQ(encoded[0], 's');  // Short form tag
}

TEST(BasonCodecTest, LongFormUsed)
{
    // Long key or value requires long form
    std::string longValue(100, 'x');
    TBasonRecord record(EBasonType::String, "key", longValue);
    auto encoded = EncodeBason(record);
    
    // Tag (1) + val_len (4) + key_len (1) + key (3) + value (100) = 109 bytes
    EXPECT_EQ(encoded.size(), 109);
    EXPECT_EQ(encoded[0], 'S');  // Long form tag
}

TEST(BasonCodecTest, EncodeDecodeArray)
{
    TBasonRecord array(EBasonType::Array, "scores", "");
    array.Children.push_back(TBasonRecord(EBasonType::Number, "0", "95"));
    array.Children.push_back(TBasonRecord(EBasonType::Number, "1", "87"));
    
    auto encoded = EncodeBason(array);
    auto [decoded, size] = DecodeBason(encoded.data(), encoded.size());
    
    EXPECT_EQ(decoded.Type, EBasonType::Array);
    EXPECT_EQ(decoded.Key, "scores");
    ASSERT_EQ(decoded.Children.size(), 2);
    EXPECT_EQ(decoded.Children[0].Value, "95");
    EXPECT_EQ(decoded.Children[1].Value, "87");
}

TEST(BasonCodecTest, EncodeDecodeObject)
{
    TBasonRecord object(EBasonType::Object, "", "");
    object.Children.push_back(TBasonRecord(EBasonType::String, "name", "Alice"));
    object.Children.push_back(TBasonRecord(EBasonType::Number, "age", "25"));
    
    auto encoded = EncodeBason(object);
    auto [decoded, size] = DecodeBason(encoded.data(), encoded.size());
    
    EXPECT_EQ(decoded.Type, EBasonType::Object);
    ASSERT_EQ(decoded.Children.size(), 2);
    EXPECT_EQ(decoded.Children[0].Key, "name");
    EXPECT_EQ(decoded.Children[0].Value, "Alice");
    EXPECT_EQ(decoded.Children[1].Key, "age");
    EXPECT_EQ(decoded.Children[1].Value, "25");
}

TEST(BasonCodecTest, DecodeAll)
{
    TBasonRecord rec1(EBasonType::String, "a", "1");
    TBasonRecord rec2(EBasonType::String, "b", "2");
    
    auto enc1 = EncodeBason(rec1);
    auto enc2 = EncodeBason(rec2);
    
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), enc1.begin(), enc1.end());
    combined.insert(combined.end(), enc2.begin(), enc2.end());
    
    auto records = DecodeBasonAll(combined.data(), combined.size());
    
    ASSERT_EQ(records.size(), 2);
    EXPECT_EQ(records[0].Key, "a");
    EXPECT_EQ(records[1].Key, "b");
}

TEST(BasonCodecTest, RoundTrip)
{
    TBasonRecord original(EBasonType::Object, "", "");
    original.Children.push_back(TBasonRecord(EBasonType::String, "name", "Bob"));
    original.Children.push_back(TBasonRecord(EBasonType::Number, "score", "100"));
    
    auto encoded = EncodeBason(original);
    auto [decoded, _] = DecodeBason(encoded.data(), encoded.size());
    auto reencoded = EncodeBason(decoded);
    
    EXPECT_EQ(encoded, reencoded);
}

////////////////////////////////////////////////////////////////////////////////
