#include "codec/record.hpp"

#include "level/level.hpp"

#include "util/util.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <format>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace bason_db;

namespace {

    class BasonLevelTest: public ::testing::Test {
    protected:
        void SetUp() override {
            const auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            auto test_dir =
                std::string{test_info->test_suite_name()} + "_" + std::string{test_info->name()};
            test_dir_ = get_test_tmp_dir() / test_dir;
            if (std::filesystem::exists(test_dir_)) {
                std::filesystem::remove_all(test_dir_);
            }
            std::filesystem::create_directories(test_dir_);
        }

        void TearDown() override {
            if (std::filesystem::exists(test_dir_)) {
                std::filesystem::remove_all(test_dir_);
            }
        }

        BasonLevel::Options default_opts() {
            return BasonLevel::Options{
                .dir = test_dir_.string(),
                .memtable_size = 4 * 1024, // 4 KB
                .l0_compaction_trigger = 4,
                .level_size_base = 64 * 1024,
                .level_size_ratio = 10,
                .max_levels = 7,
                .block_size = 512,
                .bloom_bits_per_key = 10,
            };
        }

        std::filesystem::path test_dir_;
    };

} // namespace

TEST_F(BasonLevelTest, Emtpy) {
    auto db = BasonLevel::open(default_opts());

    EXPECT_EQ(db->get("key"), std::nullopt);
}

TEST_F(BasonLevelTest, PutAndGet) {
    auto db = BasonLevel::open(default_opts());

    db->put("key1", make_string_record("key1", "value1"));
    db->put("key2", make_string_record("key2", "value2"));

    auto r1 = db->get("key1");
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->value, "value1");

    auto r2 = db->get("key2");
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->value, "value2");

    auto missing = db->get("nonexistent");
    EXPECT_FALSE(missing.has_value());

    db->close();
}

TEST_F(BasonLevelTest, UpdateKey) {
    auto db = BasonLevel::open(default_opts());

    db->put("key", make_string_record("key", "value1"));
    db->put("key", make_string_record("key", "value2"));

    auto record = db->get("key");
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->value, "value2");

    db->close();
}

TEST_F(BasonLevelTest, DeleteKey) {
    auto db = BasonLevel::open(default_opts());

    db->put("key", make_string_record("key", "value"));
    auto record = db->get("key");
    ASSERT_TRUE(record.has_value());

    db->del("key");
    auto deleted = db->get("key");
    EXPECT_FALSE(deleted.has_value());

    db->close();
}

TEST_F(BasonLevelTest, RecoveryAfterClose) {
    auto opts = default_opts();

    {
        auto db = BasonLevel::open(opts);
        db->put("persist_key", make_string_record("persist_key", "persist_value"));
        db->close();
    }

    {
        auto db = BasonLevel::open(opts);
        auto record = db->get("persist_key");
        ASSERT_TRUE(record.has_value());
        EXPECT_EQ(record->value, "persist_value");
        db->close();
    }
}

TEST_F(BasonLevelTest, DeleteRecovery) {
    auto opts = default_opts();

    {
        auto db = BasonLevel::open(opts);
        db->put("key", make_string_record("key", "value"));
        db->del("key");
        db->close();
    }

    {
        auto db = BasonLevel::open(opts);
        auto record = db->get("key");
        EXPECT_FALSE(record.has_value());
        db->close();
    }
}

TEST_F(BasonLevelTest, ManyKeys) {
    auto db = BasonLevel::open(default_opts());

    constexpr int kNumKeys = 200;
    for (int i = 0; i < kNumKeys; ++i) {
        auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
        auto val = "val_" + std::to_string(i);
        db->put(key, make_string_record(key, val));
    }

    for (int i = 0; i < kNumKeys; ++i) {
        auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
        auto val = "val_" + std::to_string(i);
        auto record = db->get(key);
        ASSERT_TRUE(record.has_value());
        EXPECT_EQ(record->value, val);
    }

    db->close();
}

TEST_F(BasonLevelTest, ManyKeysRecovery) {
    auto opts = default_opts();
    constexpr int kNumKeys = 100;

    {
        auto db = BasonLevel::open(opts);
        for (int i = 0; i < kNumKeys; ++i) {
            auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
            auto val = "val_" + std::to_string(i);
            db->put(key, make_string_record(key, val));
        }
        db->close();
    }

    {
        auto db = BasonLevel::open(opts);
        for (int i = 0; i < kNumKeys; ++i) {
            auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
            auto val = "val_" + std::to_string(i);
            auto record = db->get(key);
            ASSERT_TRUE(record.has_value());
            EXPECT_EQ(record->value, val);
        }
        db->close();
    }
}

TEST_F(BasonLevelTest, RangeScan) {
    auto db = BasonLevel::open(default_opts());

    db->put("a", make_string_record("a", "1"));
    db->put("b", make_string_record("b", "2"));
    db->put("c", make_string_record("c", "3"));
    db->put("d", make_string_record("d", "4"));
    db->put("e", make_string_record("e", "5"));

    auto it = db->scan("b", "d");
    ASSERT_TRUE(it->valid());
    EXPECT_EQ(it->key(), "b");
    it->next();
    ASSERT_TRUE(it->valid());
    EXPECT_EQ(it->key(), "c");
    it->next();
    EXPECT_FALSE(it->valid()); // "d" is exclusive

    db->close();
}

TEST_F(BasonLevelTest, FullScan) {
    auto db = BasonLevel::open(default_opts());

    auto keys = std::vector<std::string>{"a", "b", "c", "d", "e"};
    for (const auto& k : keys) {
        db->put(k, make_string_record(k, k + "_val"));
    }

    auto it = db->scan();
    auto scanned = std::vector<std::string>{};
    while (it->valid()) {
        scanned.emplace_back(it->key());
        it->next();
    }
    EXPECT_EQ(scanned, keys);

    db->close();
}

TEST_F(BasonLevelTest, Metrics) {
    auto db = BasonLevel::open(default_opts());

    db->put("key", make_string_record("key", "value"));

    auto m = db->metrics();
    EXPECT_GT(m.user_bytes_written, 0U);

    db->close();
}

TEST_F(BasonLevelTest, ForceCompaction) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    auto db = BasonLevel::open(opts);

    for (int i = 0; i < 50; ++i) {
        auto key = "key_" + std::string(4 - std::to_string(i).size(), '0') + std::to_string(i);
        auto val = std::string(64, 'x');
        db->put(key, make_string_record(key, val));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    db->compact_level(0);

    for (int i = 0; i < 50; ++i) {
        auto key = "key_" + std::string(4 - std::to_string(i).size(), '0') + std::to_string(i);
        auto record = db->get(key);
        ASSERT_TRUE(record.has_value());
    }

    db->close();
}

TEST_F(BasonLevelTest, SnapshotBasic) {
    auto db = BasonLevel::open(default_opts());

    db->put("key1", make_string_record("key1", "value1"));

    auto s1 = db->snapshot();

    db->put("key1", make_string_record("key1", "value2"));

    ASSERT_EQ(db->get("key1")->value, "value2");

    auto record = db->get("key1", ReadOptions{
                                      .snapshot = s1.get(),
                                  });

    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->value, "value1");

    db->close();
}

TEST_F(BasonLevelTest, SnapshotIdentical) {
    auto db = BasonLevel::open(default_opts());

    db->put("key1", make_string_record("key1", "value1"));

    auto s1 = db->snapshot();
    auto s2 = db->snapshot();
    auto s3 = db->snapshot();

    db->put("key1", make_string_record("key1", "value2"));

    ASSERT_EQ(db->get("key1",
                      ReadOptions{
                          .snapshot = s1.get(),
                      })
                  ->value,
              "value1");
    ASSERT_EQ(db->get("key1",
                      ReadOptions{
                          .snapshot = s2.get(),
                      })
                  ->value,
              "value1");
    ASSERT_EQ(db->get("key1",
                      ReadOptions{
                          .snapshot = s3.get(),
                      })
                  ->value,
              "value1");

    s1.reset();
    ASSERT_EQ(db->get("key1",
                      ReadOptions{
                          .snapshot = s2.get(),
                      })
                  ->value,
              "value1");
    ASSERT_EQ(db->get("key1",
                      ReadOptions{
                          .snapshot = s3.get(),
                      })
                  ->value,
              "value1");

    s2.reset();
    ASSERT_EQ(db->get("key1",
                      ReadOptions{
                          .snapshot = s3.get(),
                      })
                  ->value,
              "value1");

    ASSERT_EQ(db->get("key1")->value, "value2");

    db->close();
}

TEST_F(BasonLevelTest, SnapshotScan) {
    auto db = BasonLevel::open(default_opts());

    auto snap_empty = db->snapshot();

    db->put("a", make_string_record("a", "1"));
    db->put("b", make_string_record("b", "2"));

    auto it = db->scan("", "",
                       ReadOptions{
                           .snapshot = snap_empty.get(),
                       });
    EXPECT_FALSE(it->valid());

    auto it2 = db->scan();
    ASSERT_TRUE(it2->valid());
    EXPECT_EQ(it2->key(), "a");
    it2->next();
    ASSERT_TRUE(it2->valid());
    EXPECT_EQ(it2->key(), "b");
    it2->next();
    EXPECT_FALSE(it2->valid());

    db->close();
}

TEST_F(BasonLevelTest, SnapshotAfterFlush) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    auto db = BasonLevel::open(opts);

    db->put("key", make_string_record("key", "value1"));

    auto snapshot = db->snapshot();

    for (int i = 0; i < 30; ++i) {
        auto k = "pad_" + std::to_string(i);
        db->put(k, make_string_record(k, std::string(64, 'x')));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    db->put("key", make_string_record("key", "value2"));

    auto record = db->get("key", ReadOptions{
                                     .snapshot = snapshot.get(),
                                 });
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->value, "value1");

    ASSERT_EQ(db->get("key")->value, "value2");

    db->close();
}

TEST_F(BasonLevelTest, CompactionPreservesAllData) {
    auto opts = default_opts();
    opts.memtable_size = 512;
    opts.l0_compaction_trigger = 2;
    auto db = BasonLevel::open(opts);

    constexpr int kNumKeys = 100;
    for (int i = 0; i < kNumKeys; ++i) {
        auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
        auto val = "val_" + std::to_string(i);
        db->put(key, make_string_record(key, val));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    for (int level = 0; level < 3; ++level) {
        db->compact_level(level);
    }

    auto metrics = db->metrics();
    EXPECT_GE(metrics.total_compactions, 3U);

    for (int i = 0; i < kNumKeys; ++i) {
        auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
        auto val = "val_" + std::to_string(i);
        auto record = db->get(key);
        ASSERT_TRUE(record.has_value());
        EXPECT_EQ(record->value, val);
    }

    db->close();
}

TEST_F(BasonLevelTest, RepeatedWritesToSameKey) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    opts.l0_compaction_trigger = 4;
    auto db = BasonLevel::open(opts);

    auto value = std::string(128, 'x');

    constexpr int kIterations = 50;

    for (int i = 0; i < kIterations; ++i) {
        db->put("key", make_string_record("key", value));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    db->compact_level(0);

    auto record = db->get("key");
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->value, value);

    auto metrics = db->metrics();
    EXPECT_GT(metrics.total_compactions, 0U);

    db->close();
}

TEST_F(BasonLevelTest, DeleteMarkersRemovedByCompaction) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    auto db = BasonLevel::open(opts);

    db->put("key", make_string_record("key", "value"));
    db->del("key");

    constexpr int kIterations = 50;

    for (int i = 0; i < kIterations; ++i) {
        auto k = "pad_" + std::to_string(i);
        db->put(k, make_string_record(k, std::string(64, 'y')));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    db->compact_level(0);

    auto record = db->get("key");
    EXPECT_FALSE(record.has_value());

    auto metrics = db->metrics();
    EXPECT_GT(metrics.total_compactions, 0U);

    db->close();
}

TEST_F(BasonLevelTest, L0Ordering) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    auto db = BasonLevel::open(opts);

    db->put("key1", make_string_record("key1", "value1"));
    db->put("key2", make_string_record("key2", "value2"));

    constexpr int kIterations = 50;

    for (int i = 0; i < kIterations; ++i) {
        auto k = "pad1_" + std::to_string(i);
        db->put(k, make_string_record(k, std::string(64, 'a')));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    db->put("key2", make_string_record("key2", "value3"));
    for (int i = 0; i < kIterations; ++i) {
        auto k = "pad2_" + std::to_string(i);
        db->put(k, make_string_record(k, std::string(64, 'b')));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto record = db->get("key2");
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->value, "value3");

    db->close();
}

TEST_F(BasonLevelTest, MultiLevelCompaction) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    opts.l0_compaction_trigger = 2;
    opts.level_size_base = 1024;
    opts.level_size_ratio = 2;
    auto db = BasonLevel::open(opts);

    constexpr int kIterations = 200;

    for (int i = 0; i < kIterations; ++i) {
        auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
        db->put(key, make_string_record(key, std::string(32, 'v')));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    db->compact_level(0);
    db->compact_level(1);

    for (int i = 0; i < kIterations; ++i) {
        auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
        auto record = db->get(key);
        ASSERT_TRUE(record.has_value());
    }

    auto metrics = db->metrics();
    EXPECT_GE(metrics.total_compactions, 3U);

    db->close();
}

TEST_F(BasonLevelTest, RecoveryAfterWrites) {
    auto opts = default_opts();
    opts.memtable_size = 512;

    constexpr int kIterations = 200;

    {
        auto db = BasonLevel::open(opts);
        for (int i = 0; i < kIterations; ++i) {
            auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
            db->put(key, make_string_record(key, std::string(64, 'x')));
        }
        db->close();
    }

    {
        auto db = BasonLevel::open(opts);
        for (int i = 0; i < kIterations; ++i) {
            auto key = "key_" + std::string(5 - std::to_string(i).size(), '0') + std::to_string(i);
            auto record = db->get(key);
            ASSERT_TRUE(record.has_value());
        }
        db->close();
    }
}

TEST_F(BasonLevelTest, CompactionAfterDelete) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    auto db = BasonLevel::open(opts);

    constexpr int kIterations = 50;

    for (int i = 0; i < kIterations; ++i) {
        auto key = "key_" + std::string(4 - std::to_string(i).size(), '0') + std::to_string(i);
        db->put(key, make_string_record(key, "val_" + std::to_string(i)));
    }

    for (int i = 0; i < kIterations; i += 2) {
        auto key = "key_" + std::string(4 - std::to_string(i).size(), '0') + std::to_string(i);
        db->del(key);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    db->compact_level(0);

    auto metrics = db->metrics();
    EXPECT_GT(metrics.total_compactions, 0U);

    for (int i = 0; i < kIterations; ++i) {
        auto key = "key_" + std::string(4 - std::to_string(i).size(), '0') + std::to_string(i);
        auto record = db->get(key);
        if (i % 2 == 0) {
            EXPECT_FALSE(record.has_value());
        } else {
            ASSERT_TRUE(record.has_value());
            EXPECT_EQ(record->value, "val_" + std::to_string(i));
        }
    }

    db->close();
}

TEST_F(BasonLevelTest, ScanAfterCompaction) {
    auto opts = default_opts();
    opts.memtable_size = 256;
    auto db = BasonLevel::open(opts);

    auto keys = std::vector<std::string>{"a", "b", "c", "d", "e"};
    for (const auto& key : keys) {
        db->put(key, make_string_record(key, key + "_val"));
    }

    constexpr int kIterations = 50;

    for (int i = 0; i < kIterations; ++i) {
        auto k = "zzz_pad_" + std::to_string(i);
        db->put(k, make_string_record(k, std::string(64, 'p')));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    db->compact_level(0);

    auto metrics = db->metrics();
    EXPECT_GT(metrics.total_compactions, 0U);

    auto it = db->scan("a", "f");
    auto scanned = std::vector<std::string>{};
    while (it->valid()) {
        scanned.emplace_back(it->key());
        it->next();
    }
    EXPECT_EQ(scanned, keys);

    db->close();
}

TEST_F(BasonLevelTest, MultiThreaded) {
    auto opts = default_opts();
    opts.memtable_size = 1024;
    auto db = BasonLevel::open(opts);

    constexpr int kNumThreads = 4;
    constexpr int kOpsPerThread = 200;
    constexpr int kNumKeys = 50;

    std::atomic_bool stop = false;
    std::atomic_int errors = 0;

    auto worker = [&](int id) {
        auto rng = std::mt19937{static_cast<unsigned>(1000 + id)};
        auto key_dist = std::uniform_int_distribution<int>{0, kNumKeys - 1};
        auto op_dist = std::uniform_int_distribution<int>{0, 99};

        for (int op = 0; op < kOpsPerThread && !stop.load(); ++op) {
            int key_idx = key_dist(rng);

            auto oss = std::ostringstream{};
            oss << "key_" << std::setw(4) << std::setfill('0') << key_idx;

            auto key = oss.str();

            int p = op_dist(rng);
            if (p < 50) {
                // Write
                auto oss = std::ostringstream{};
                oss << "key_" << std::setw(4) << std::setfill('0') << key_idx;

                try {
                    db->put(key, make_string_record(key, std::format("{}.{}", id, op)));
                } catch (const std::exception& e) {
                    errors.fetch_add(1);
                }
            } else {
                // Read
                try {
                    [[maybe_unused]] auto record = db->get(key);
                } catch (const std::exception& e) {
                    errors.fetch_add(1);
                }
            }
        }
    };

    auto threads = std::vector<std::jthread>{};
    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(errors.load(), 0);

    for (int i = 0; i < kNumKeys; ++i) {
        auto oss = std::ostringstream{};
        oss << "key_" << std::setw(4) << std::setfill('0') << i;

        auto key = oss.str();

        [[maybe_unused]] auto r = db->get(key);
    }

    db->close();
}

TEST_F(BasonLevelTest, SnapshotWithConcurrentWrites) {
    auto opts = default_opts();
    opts.memtable_size = 1024;
    auto db = BasonLevel::open(opts);

    db->put("key", make_string_record("key", "value"));

    auto snapshot = db->snapshot();

    constexpr int kNumWriters = 3;
    constexpr int kOpsPerWriter = 100;
    std::atomic_int errors = 0;

    auto writer = [&](int id) {
        for (int i = 0; i < kOpsPerWriter; ++i) {
            try {
                db->put("key", make_string_record("key", "w" + std::to_string(id) + "_" +
                                                             std::to_string(i)));
            } catch (...) {
                errors.fetch_add(1);
            }
        }
    };

    auto threads = std::vector<std::jthread>{};
    for (int i = 0; i < kNumWriters; ++i) {
        threads.emplace_back(writer, i);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto record = db->get("key", ReadOptions{
                                     .snapshot = snapshot.get(),
                                 });
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->value, "value");

    auto current = db->get("key");
    ASSERT_TRUE(current.has_value());
    EXPECT_NE(current->value, "value");

    EXPECT_EQ(errors.load(), 0);

    db->close();
}
