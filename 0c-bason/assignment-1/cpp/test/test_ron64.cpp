#include "../src/ron64.h"

#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

TEST(Ron64Test, EncodeZero)
{
    EXPECT_EQ(EncodeRon64(0), "0");
}

TEST(Ron64Test, EncodeSingleDigit)
{
    EXPECT_EQ(EncodeRon64(9), "9");
    EXPECT_EQ(EncodeRon64(10), "A");
    EXPECT_EQ(EncodeRon64(36), "_");
    EXPECT_EQ(EncodeRon64(63), "~");
}

TEST(Ron64Test, EncodeMultiDigit)
{
    EXPECT_EQ(EncodeRon64(64), "10");
    EXPECT_EQ(EncodeRon64(100), "1_");  // 100 = 1*64 + 36, где 36='_'
    EXPECT_EQ(EncodeRon64(4095), "~~");
    EXPECT_EQ(EncodeRon64(4096), "100");
}

TEST(Ron64Test, DecodeZero)
{
    EXPECT_EQ(DecodeRon64("0"), 0);
}

TEST(Ron64Test, DecodeSingleDigit)
{
    EXPECT_EQ(DecodeRon64("9"), 9);
    EXPECT_EQ(DecodeRon64("A"), 10);
    EXPECT_EQ(DecodeRon64("_"), 36);
    EXPECT_EQ(DecodeRon64("~"), 63);
}

TEST(Ron64Test, DecodeMultiDigit)
{
    EXPECT_EQ(DecodeRon64("10"), 64);
    EXPECT_EQ(DecodeRon64("1_"), 100);  // Исправлено: '_' на позиции 36
    EXPECT_EQ(DecodeRon64("~~"), 4095);
    EXPECT_EQ(DecodeRon64("100"), 4096);
}

TEST(Ron64Test, RoundTrip)
{
    for (uint64_t value : {0, 1, 10, 63, 64, 100, 1000, 10000, 1000000}) {
        std::string encoded = EncodeRon64(value);
        uint64_t decoded = DecodeRon64(encoded);
        EXPECT_EQ(decoded, value) << "Failed for value " << value;
    }
}

TEST(Ron64Test, LexicographicOrder)
{
    // RON64 encoding should preserve numeric order lexicographically
    // BUT only within numbers of the SAME length!
    // Single-digit range: 0-63
    for (uint64_t a = 0; a < 63; ++a) {
        for (uint64_t b = a + 1; b < 64; ++b) {
            std::string encA = EncodeRon64(a);
            std::string encB = EncodeRon64(b);
            EXPECT_LT(encA, encB) << "Order violation: " << a << " vs " << b;
        }
    }
    
    // Two-digit range: 64-4095
    for (uint64_t a = 64; a < 200; a += 7) {
        for (uint64_t b = a + 1; b < 200; b += 7) {
            std::string encA = EncodeRon64(a);
            std::string encB = EncodeRon64(b);
            EXPECT_LT(encA, encB) << "Order violation: " << a << " vs " << b;
        }
    }
}

TEST(Ron64Test, InvalidCharacter)
{
    EXPECT_THROW(DecodeRon64("@"), std::invalid_argument);
    EXPECT_THROW(DecodeRon64("!"), std::invalid_argument);
    EXPECT_THROW(DecodeRon64(" "), std::invalid_argument);
}

TEST(Ron64Test, EmptyString)
{
    EXPECT_THROW(DecodeRon64(""), std::invalid_argument);
}

TEST(Ron64Test, MinimalEncoding)
{
    for (uint64_t value = 0; value <= 10000; ++value) {
        std::string enc = EncodeRon64(value);
        if (value == 0) {
            EXPECT_EQ(enc, "0") << "Zero must encode to single '0'";
        } else {
            ASSERT_FALSE(enc.empty());
            EXPECT_NE(enc[0], '0')
                << "Non-zero value " << value << " must not have leading zero, got " << enc;
        }
    }
    // Spot-check larger values
    for (uint64_t value : {100000ull, 1000000ull, UINT64_MAX}) {
        std::string enc = EncodeRon64(value);
        ASSERT_FALSE(enc.empty());
        if (value != 0)
            EXPECT_NE(enc[0], '0') << "Value " << value << " encoded as " << enc;
    }
}

TEST(Ron64Test, RoundTripLargeRange)
{
    for (uint64_t value = 0; value < 100000; ++value) {
        std::string encoded = EncodeRon64(value);
        uint64_t decoded = DecodeRon64(encoded);
        EXPECT_EQ(decoded, value) << "Round-trip failed for value " << value;
    }
    // Sparse check at powers of two and near max
    for (uint64_t value : {1ull, 63ull, 64ull, 4095ull, 4096ull, 1ull << 20, 1ull << 24,
                           1ull << 32, 1ull << 40, 1ull << 48, UINT64_MAX - 1, UINT64_MAX}) {
        uint64_t decoded = DecodeRon64(EncodeRon64(value));
        EXPECT_EQ(decoded, value) << "Round-trip failed for value " << value;
    }
}

////////////////////////////////////////////////////////////////////////////////
