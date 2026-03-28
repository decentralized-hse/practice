#include "codec/record.hpp"

#include <gtest/gtest.h>

using namespace bason_db;

TEST(BasonCodecTest, ShortFormLeaf) {
    auto original = BasonRecord{
        .type = BasonType::String,
        .key = "name",
        .value = "Alice",
        .children = {},
    };

    auto encoded = bason_encode(original);

    EXPECT_EQ(encoded[0], 's');

    auto [decoded, consumed] = bason_decode(encoded.data(), encoded.size());

    EXPECT_EQ(consumed, encoded.size());
    EXPECT_EQ(decoded, original);
}

TEST(BasonCodecTest, LongFormLeaf) {
    auto original = BasonRecord{
        .type = BasonType::String,
        .key = "very_long_key_name_for_testing",
        .value = "A very long value string that definitely exceeds constraints",
        .children = {},
    };

    auto encoded = bason_encode(original);

    EXPECT_EQ(encoded[0], 'S');

    auto [decoded, consumed] = bason_decode(encoded.data(), encoded.size());

    EXPECT_EQ(consumed, encoded.size());
    EXPECT_EQ(decoded, original);
}

TEST(BasonCodecTest, NestedObjectForm) {
    auto child1 = BasonRecord{
        .type = BasonType::Number,
        .key = "id",
        .value = "42",
        .children = {},
    };

    auto child2 = BasonRecord{
        .type = BasonType::Boolean,
        .key = "admin",
        .value = "true",
        .children = {},
    };

    auto original = BasonRecord{
        .type = BasonType::Object,
        .key = "user",
        .value = "",
        .children =
            {
                child1,
                child2,
            },
    };

    auto encoded = bason_encode(original);
    auto [decoded, consumed] = bason_decode(encoded.data(), encoded.size());

    EXPECT_EQ(consumed, encoded.size());
    EXPECT_EQ(decoded, original);
    EXPECT_EQ(decoded.children.size(), 2);
    EXPECT_EQ(decoded.children[0].key, "id");
    EXPECT_EQ(decoded.children[1].value, "true");
}

TEST(BasonCodecTest, HandlesTruncatedBuffer) {
    auto original = BasonRecord{
        .type = BasonType::String,
        .key = "name",
        .value = "Alice",
        .children = {},
    };

    auto encoded = bason_encode(original);

    EXPECT_THROW(bason_decode(encoded.data(), encoded.size() - 2), std::runtime_error);

    EXPECT_THROW(bason_decode(encoded.data(), 1), std::runtime_error);
}
