#include "../src/json_converter.h"

#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

TEST(JsonConverterTest, StringValue)
{
    std::string json = R"({"name": "Alice"})";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"({"name":"Alice"})");
}

TEST(JsonConverterTest, NumberValue)
{
    std::string json = R"({"age": 25})";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"({"age":25})");
}

TEST(JsonConverterTest, BooleanValues)
{
    std::string json = R"({"active": true, "deleted": false})";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"({"active":true,"deleted":false})");
}

TEST(JsonConverterTest, NullValue)
{
    std::string json = R"({"value": null})";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"({"value":null})");
}

TEST(JsonConverterTest, Array)
{
    std::string json = R"([1, 2, 3])";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"([1,2,3])");
}

TEST(JsonConverterTest, NestedObject)
{
    std::string json = R"({"user": {"name": "Alice", "age": 25}})";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"({"user":{"name":"Alice","age":25}})");
}

TEST(JsonConverterTest, ArrayOfObjects)
{
    std::string json = R"([{"x": 1}, {"x": 2}])";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"([{"x":1},{"x":2}])");
}

TEST(JsonConverterTest, EscapeSequences)
{
    std::string json = R"({"text": "line1\nline2\ttab"})";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"({"text":"line1\nline2\ttab"})");
}

TEST(JsonConverterTest, EmptyObject)
{
    std::string json = R"({})";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"({})");
}

TEST(JsonConverterTest, EmptyArray)
{
    std::string json = R"([])";
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    EXPECT_EQ(result, R"([])");
}

TEST(JsonConverterTest, ComplexExample)
{
    std::string json = R"({
        "name": "Alice",
        "age": 25,
        "scores": [95, 87, 92],
        "active": true,
        "address": {
            "city": "NYC",
            "zip": "10001"
        }
    })";
    
    auto bason = JsonToBason(json);
    std::string result = BasonToJson(bason.data(), bason.size());
    
    // Verify it's valid JSON by parsing again
    auto bason2 = JsonToBason(result);
    EXPECT_FALSE(bason2.empty());
}

////////////////////////////////////////////////////////////////////////////////
