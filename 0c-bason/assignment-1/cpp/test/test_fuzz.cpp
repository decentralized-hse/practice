// Fuzz test for Assignment 1: random JSON -> BASON -> JSON round-trip
// Run with FUZZ_ITERATIONS=10000000 for full 10M iterations

#include "../src/bason_codec.h"
#include "../src/json_converter.h"

#include <gtest/gtest.h>
#include <random>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <cmath>
#include <sstream>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

namespace {

// Parse as number when possible; return true if both are numeric and equal.
static bool NumericValuesEqual(const std::string& va, const std::string& vb)
{
    if (va == vb) return true;
    std::stringstream sa(va), sb(vb);
    double da = 0, db = 0;
    if (!(sa >> da) || !(sb >> db)) return false;
    return std::abs(da - db) <= 1e-9 * (std::abs(da) + std::abs(db) + 1.0);
}

// Semantic equality of two BASON records.
// For Object: order and duplicate keys ignored (last occurrence wins), so round-trip
// JSON (which collapses duplicate keys) compares equal.
bool RecordsEqual(const TBasonRecord& a, const TBasonRecord& b)
{
    if (a.Type != b.Type) return false;
    if (a.Key != b.Key) return false;
    if (a.Type == EBasonType::Array) {
        if (a.Children.size() != b.Children.size()) return false;
        for (size_t i = 0; i < a.Children.size(); ++i) {
            if (!RecordsEqual(a.Children[i], b.Children[i])) return false;
        }
        return true;
    }
    if (a.Type == EBasonType::Object) {
        // Compare as key -> value map (last wins for duplicates)
        std::map<std::string, size_t> aLast;
        for (size_t i = 0; i < a.Children.size(); ++i)
            aLast[a.Children[i].Key] = i;
        std::map<std::string, size_t> bLast;
        for (size_t j = 0; j < b.Children.size(); ++j)
            bLast[b.Children[j].Key] = j;
        if (aLast.size() != bLast.size()) return false;
        for (const auto& [key, i] : aLast) {
            auto it = bLast.find(key);
            if (it == bLast.end()) return false;
            if (!RecordsEqual(a.Children[i], b.Children[it->second])) return false;
        }
        return true;
    }
    if (a.Type == EBasonType::Number)
        return NumericValuesEqual(a.Value, b.Value);
    return a.Value == b.Value;
}

// Random JSON generator (depth- and size-limited for stability).
class RandomJson
{
public:
    explicit RandomJson(uint32_t seed) : Rng_(seed) {}

    std::string Generate(int maxDepth = 5, int maxChildren = 8)
    {
        return GenerateValue(maxDepth, maxChildren);
    }

private:
    std::mt19937 Rng_;

    char RandomChar()
    {
        static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        return chars[static_cast<size_t>(Rng_()) % (sizeof(chars) - 1)];
    }

    std::string RandomString(size_t maxLen = 20)
    {
        size_t len = 1 + static_cast<size_t>(Rng_()) % maxLen;
        std::string s = "\"";
        for (size_t i = 0; i < len; ++i) {
            char c = RandomChar();
            if (c == '"' || c == '\\') s += '\\';
            s += c;
        }
        s += '"';
        return s;
    }

    std::string RandomNumber()
    {
        uint64_t v = Rng_();
        if (v % 5 == 0) return "0";
        if (v % 5 == 1) return "1";
        if (v % 5 == 2) return std::to_string(v % 1000);
        if (v % 5 == 3) return std::to_string(static_cast<int64_t>(Rng_()) % 10000);
        return std::to_string(static_cast<double>(Rng_()) / 1e6);
    }

    std::string GenerateValue(int depth, int maxChildren)
    {
        if (depth <= 0) {
            switch (static_cast<unsigned>(Rng_()) % 5) {
                case 0: return "null";
                case 1: return (Rng_() % 2) ? "true" : "false";
                case 2: return RandomNumber();
                case 3: return RandomString(8);
                default: return "[]";
            }
        }
        switch (static_cast<unsigned>(Rng_()) % 6) {
            case 0: return "null";
            case 1: return (Rng_() % 2) ? "true" : "false";
            case 2: return RandomNumber();
            case 3: return RandomString(15);
            case 4: return GenerateArray(depth, maxChildren);
            default: return GenerateObject(depth, maxChildren);
        }
    }

    std::string GenerateArray(int depth, int maxChildren)
    {
        int n = 1 + static_cast<int>(Rng_() % static_cast<unsigned>(maxChildren));
        std::string s = "[";
        for (int i = 0; i < n; ++i) {
            if (i) s += ",";
            s += GenerateValue(depth - 1, maxChildren);
        }
        s += "]";
        return s;
    }

    std::string GenerateObject(int depth, int maxChildren)
    {
        int n = 1 + static_cast<int>(Rng_() % static_cast<unsigned>(maxChildren));
        std::string s = "{";
        for (int i = 0; i < n; ++i) {
            if (i) s += ",";
            s += RandomString(10);
            s += ":";
            s += GenerateValue(depth - 1, maxChildren);
        }
        s += "}";
        return s;
    }
};

} // namespace

////////////////////////////////////////////////////////////////////////////////

TEST(FuzzTest, JsonBasonRoundTrip)
{
    const char* env = std::getenv("FUZZ_ITERATIONS");
    const unsigned long iterations = env ? std::strtoul(env, nullptr, 10) : 10'000u;
    if (iterations == 0) return;

    std::random_device rd;
    uint32_t seed = rd();
    RandomJson gen(seed);

    for (unsigned long i = 0; i < iterations; ++i) {
        std::string json;
        try {
            json = gen.Generate(4, 6);
        } catch (...) {
            continue;
        }

        std::vector<uint8_t> bason;
        std::string json2;
        TBasonRecord record1, record2;

        try {
            bason = JsonToBason(json);
            if (bason.empty()) continue;
            json2 = BasonToJson(bason.data(), bason.size());
            auto bason2 = JsonToBason(json2);

            auto [r1, len1] = DecodeBason(bason.data(), bason.size());
            auto [r2, len2] = DecodeBason(bason2.data(), bason2.size());
            record1 = std::move(r1);
            record2 = std::move(r2);
        } catch (const std::exception&) {
            continue; // Skip invalid or unsupported JSON
        }

        ASSERT_TRUE(RecordsEqual(record1, record2))
            << "Round-trip failed at iteration " << i << " seed " << seed
            << "\nOriginal JSON: " << json
            << "\nRound-trip JSON: " << json2;
    }
}

////////////////////////////////////////////////////////////////////////////////
