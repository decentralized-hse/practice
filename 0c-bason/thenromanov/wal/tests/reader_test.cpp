#include "codec/record.hpp"

#include "wal/constants.hpp"
#include "wal/reader.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace bason_db;

namespace {

    class WalReaderTest: public ::testing::Test {
    protected:
        void SetUp() override {
            const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            const auto test_dir =
                std::string{test_info->test_suite_name()} + "_" + std::string{test_info->name()};
            test_dir_ = get_test_tmp_dir() / test_dir;
            std::filesystem::create_directories(test_dir_);
        }

        void TearDown() override {
            if (std::filesystem::exists(test_dir_)) {
                std::filesystem::remove_all(test_dir_);
            }
        }

        void WriteFile(const std::string& filename, const std::vector<uint8_t>& data) {
            auto fs = std::ofstream{test_dir_ / filename, std::ios::binary};
            fs.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        std::vector<uint8_t> GenerateHeader(uint64_t start_offset = 0) {
            std::vector<uint8_t> header(kHeaderSize, 0);
            std::memcpy(header.data(), kWalTag.data(), kWalTag.size());
            header[8] = 1;
            header[9] = 0;
            header[10] = 0;
            header[11] = 0;

            std::memcpy(header.data() + 12, &start_offset, 8);
            return header;
        }

        std::vector<uint8_t> GenerateCheckpoint() {
            auto cp = std::vector<uint8_t>(5, 0);
            cp[0] = kCheckpointTag;
            return cp;
        }

        void AppendRecordToBuffer(std::vector<uint8_t>& wal_buffer, const BasonRecord& record) {
            auto encoded = bason_encode(record);
            wal_buffer.insert(wal_buffer.end(), encoded.begin(), encoded.end());

            size_t consumed = encoded.size();
            size_t padded = (consumed + 7) & ~7ULL;
            size_t padding_bytes = padded - consumed;

            if (padding_bytes > 0) {
                wal_buffer.insert(wal_buffer.end(), padding_bytes, 0x00);
            }
        }

        std::filesystem::path test_dir_;
    };

} // namespace

TEST_F(WalReaderTest, EmptyDirectoryRecovery) {
    auto reader = WalReader::open(test_dir_);
    uint64_t safe_offset = reader.recover();
    EXPECT_EQ(safe_offset, 0);

    auto scanner = reader.scan(0);
    EXPECT_FALSE(scanner.valid());
}

TEST_F(WalReaderTest, IgnoresInvalidFiles) {
    WriteFile("0.txt", GenerateHeader());

    WriteFile("1.wal", {0x01, 0x02});

    auto bad_tag = std::vector<uint8_t>(kHeaderSize, 0x00);
    WriteFile("2.wal", bad_tag);

    auto reader = WalReader::open(test_dir_);
    EXPECT_EQ(reader.recover(), 0);

    auto scanner = reader.scan(0);
    EXPECT_FALSE(scanner.valid());
}

TEST_F(WalReaderTest, RecoversAndScansCommittedRecords) {
    auto wal_data = std::vector<uint8_t>{};

    auto header = GenerateHeader(0);
    wal_data.insert(wal_data.end(), header.begin(), header.end());

    auto record1 = make_string_record("user_id", "4095");
    AppendRecordToBuffer(wal_data, record1);

    auto cp = GenerateCheckpoint();
    wal_data.insert(wal_data.end(), cp.begin(), cp.end());

    WriteFile("00000000000000000000.wal", wal_data);

    auto reader = WalReader::open(test_dir_.string());

    EXPECT_EQ(reader.recover(), wal_data.size());

    auto scanner = reader.scan(0);
    ASSERT_TRUE(scanner.valid());

    EXPECT_EQ(scanner.offset(), kHeaderSize);
    EXPECT_EQ(scanner.record().key, "user_id");
    EXPECT_EQ(scanner.record().value, "4095");

    scanner.next();
    EXPECT_FALSE(scanner.valid());
}

TEST_F(WalReaderTest, TruncatesUncommittedData) {
    auto wal_data = std::vector<uint8_t>{};

    auto header = GenerateHeader(0);
    wal_data.insert(wal_data.end(), header.begin(), header.end());

    auto cp = GenerateCheckpoint();
    wal_data.insert(wal_data.end(), cp.begin(), cp.end());

    uint64_t safe_eof = wal_data.size();

    auto incomplete = make_string_record("incomplete_key", "incomplete_val");
    auto enc = bason_encode(incomplete);
    wal_data.insert(wal_data.end(), enc.begin(), enc.begin() + 4);

    WriteFile("00000000000000000000.wal", wal_data);

    auto reader = WalReader::open(test_dir_.string());
    uint64_t recovery_off = reader.recover();

    EXPECT_EQ(recovery_off, safe_eof);
    ASSERT_EQ(std::filesystem::file_size(test_dir_ / "00000000000000000000.wal"), safe_eof);
}

TEST_F(WalReaderTest, ScanRespectsFromOffset) {
    auto wal_data = std::vector<uint8_t>{};

    auto header = GenerateHeader(0);
    wal_data.insert(wal_data.end(), header.begin(), header.end());

    auto record1 = make_string_record("rec_a", "val_a");
    auto record2 = make_string_record("rec_b", "val_b");

    AppendRecordToBuffer(wal_data, record1);

    uint64_t offset_of_record_2 = wal_data.size();

    AppendRecordToBuffer(wal_data, record2);

    auto cp = GenerateCheckpoint();
    wal_data.insert(wal_data.end(), cp.begin(), cp.end());

    WriteFile("00000000000000000000.wal", wal_data);

    auto reader = WalReader::open(test_dir_.string());
    reader.recover();

    auto scanner = reader.scan(offset_of_record_2);

    ASSERT_TRUE(scanner.valid());
    EXPECT_EQ(scanner.offset(), offset_of_record_2);
    EXPECT_EQ(scanner.record().key, "rec_b");

    scanner.next();
    EXPECT_FALSE(scanner.valid());
}
