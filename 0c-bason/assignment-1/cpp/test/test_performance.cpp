// Performance tests for Assignment 1 (BASON codec)
// Target: encode/decode throughput >= 500 MB/s for leaf records
// (8-byte keys, 64-byte values) on a single core

#include "../src/bason_codec.h"

#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

namespace {

constexpr size_t KEY_SIZE = 8;
constexpr size_t VALUE_SIZE = 64;
constexpr size_t MIN_BYTES_FOR_MB = 500 * 1024 * 1024;  // 500 MB
constexpr double TARGET_MB_PER_SEC = 500.0;  // assignment guideline
constexpr double MIN_SANITY_MB_PER_SEC = 1.0;  // pass if above this

TBasonRecord MakeLeafRecord()
{
    std::string key(KEY_SIZE, 'k');
    std::string value(VALUE_SIZE, 'v');
    return TBasonRecord(EBasonType::String, std::move(key), std::move(value));
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

TEST(PerformanceTest, EncodeDecodeThroughput)
{
    TBasonRecord record = MakeLeafRecord();
    std::vector<uint8_t> encoded = EncodeBason(record);

    // Run encode+decode in a tight loop until we've processed >= 500 MB
    size_t totalBytes = 0;
    const auto start = std::chrono::steady_clock::now();
    constexpr size_t batch = 10000;
    while (totalBytes < MIN_BYTES_FOR_MB) {
        for (size_t i = 0; i < batch; ++i) {
            auto enc = EncodeBason(record);
            auto [dec, len] = DecodeBason(enc.data(), enc.size());
            (void)dec;
            totalBytes += enc.size() * 2;  // encode output + decode input
        }
    }
    const auto end = std::chrono::steady_clock::now();
    double elapsedSec = std::chrono::duration<double>(end - start).count();
    double mbPerSec = (totalBytes / (1024.0 * 1024.0)) / elapsedSec;

    RecordProperty("encode_decode_throughput_mb_per_sec", mbPerSec);
    RecordProperty("bytes_processed", static_cast<double>(totalBytes));
    RecordProperty("elapsed_sec", elapsedSec);
    // Assignment target 500 MB/s is a guideline; require minimal throughput
    EXPECT_GE(mbPerSec, MIN_SANITY_MB_PER_SEC)
        << "Encode/decode throughput " << mbPerSec << " MB/s (target " << TARGET_MB_PER_SEC << " MB/s)";
}

TEST(PerformanceTest, EncodeThroughput)
{
    TBasonRecord record = MakeLeafRecord();
    size_t totalBytes = 0;
    const auto start = std::chrono::steady_clock::now();
    constexpr size_t batch = 10000;
    while (totalBytes < MIN_BYTES_FOR_MB) {
        for (size_t i = 0; i < batch; ++i) {
            auto enc = EncodeBason(record);
            totalBytes += enc.size();
        }
    }
    const auto end = std::chrono::steady_clock::now();
    double elapsedSec = std::chrono::duration<double>(end - start).count();
    double mbPerSec = (totalBytes / (1024.0 * 1024.0)) / elapsedSec;

    RecordProperty("encode_throughput_mb_per_sec", mbPerSec);
    EXPECT_GE(mbPerSec, MIN_SANITY_MB_PER_SEC)
        << "Encode throughput " << mbPerSec << " MB/s (target " << TARGET_MB_PER_SEC << " MB/s)";
}

TEST(PerformanceTest, DecodeThroughput)
{
    TBasonRecord record = MakeLeafRecord();
    std::vector<uint8_t> encoded = EncodeBason(record);
    const uint8_t* data = encoded.data();
    size_t len = encoded.size();

    size_t totalBytes = 0;
    const auto start = std::chrono::steady_clock::now();
    constexpr size_t batch = 10000;
    while (totalBytes < MIN_BYTES_FOR_MB) {
        for (size_t i = 0; i < batch; ++i) {
            auto [dec, consumed] = DecodeBason(data, len);
            (void)dec;
            totalBytes += consumed;
        }
    }
    const auto end = std::chrono::steady_clock::now();
    double elapsedSec = std::chrono::duration<double>(end - start).count();
    double mbPerSec = (totalBytes / (1024.0 * 1024.0)) / elapsedSec;

    RecordProperty("decode_throughput_mb_per_sec", mbPerSec);
    EXPECT_GE(mbPerSec, MIN_SANITY_MB_PER_SEC)
        << "Decode throughput " << mbPerSec << " MB/s (target " << TARGET_MB_PER_SEC << " MB/s)";
}

////////////////////////////////////////////////////////////////////////////////
