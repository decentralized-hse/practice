#include "codec/record.hpp"

#include "level/level.hpp"

#include "util/util.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

using bason_db::BasonLevel;
using bason_db::make_string_record;
using bason_db::ReadOptions;

int main() {
    const auto db_dir = std::filesystem::temp_directory_path() / "bason_level_example";
    std::filesystem::remove_all(db_dir);

    std::cout << "Opening database at " << db_dir << "\n";

    auto db = BasonLevel::open({
        .dir = db_dir,
        .memtable_size = size_t{4} * 1024, // 4 KB
        .l0_compaction_trigger = 4,
        .level_size_base = size_t{64} * 1024, // 64 KB
        .max_levels = 7,
        .block_size = 512,
    });

    std::cout << "\n=== Basic operations ===\n";

    db->put("a", make_string_record("a", "A"));
    db->put("b", make_string_record("b", "B"));
    db->put("c", make_string_record("c", "C"));
    db->put("d", make_string_record("d", "D"));

    auto result = db->get("a");
    assert(result.has_value());
    std::cout << "get(a) -> " << result->value << "\n";

    result = db->get("f");
    assert(!result.has_value());
    std::cout << "get(f) -> not found\n";

    db->put("a", make_string_record("a", "A2"));
    result = db->get("a");
    assert(result.has_value());
    std::cout << "get(a) -> " << result->value << "\n";

    db->del("d");
    result = db->get("d");
    assert(!result.has_value());
    std::cout << "get(d) after delete -> not found\n";

    std::cout << "\n=== Range scan ===\n";

    auto iter = db->scan("a", "d");
    std::cout << "scan [a, d):\n";
    while (iter->valid()) {
        std::cout << "  key=" << iter->key() << '\n';
        iter->next();
    }

    iter = db->scan();
    std::cout << "full scan:\n";
    while (iter->valid()) {
        std::cout << "  key=" << iter->key() << '\n';
        iter->next();
    }

    std::cout << "\n=== Snapshots ===\n";

    auto snapshot = db->snapshot();
    std::cout << "snapshot created at offset " << snapshot->offset << '\n';

    db->put("a", make_string_record("a", "A3"));
    db->put("e", make_string_record("e", "E"));
    db->del("b");

    std::cout << "current view:\n";
    iter = db->scan();
    while (iter->valid()) {
        std::cout << "  key=" << iter->key() << '\n';
        iter->next();
    }

    std::cout << "snapshot view:\n";
    iter = db->scan("", "",
                    ReadOptions{
                        .snapshot = snapshot.get(),
                    });
    while (iter->valid()) {
        std::cout << "  key=" << iter->key() << '\n';
        iter->next();
    }

    result = db->get("b", ReadOptions{
                              .snapshot = snapshot.get(),
                          });
    assert(result.has_value());
    std::cout << "snapshot get(b) -> " << result->value << " (was deleted in current view)\n ";

    result = db->get("e", ReadOptions{
                              .snapshot = snapshot.get(),
                          });
    assert(!result.has_value());
    std::cout << "snapshot get(e) -> not found (added after snapshot)\n";

    snapshot.reset();
    std::cout << "snapshot released\n";

    std::cout << "\n=== Compaction & Metrics ===\n";

    for (int i = 0; i < 200; ++i) {
        auto key = "key:" + std::to_string(1000 + i);
        db->put(key, make_string_record(key, "value-" + std::to_string(i)));
    }

    db->compact_level(0);

    auto m = db->metrics();
    std::cout << "total compactions: " << m.total_compactions << '\n';
    std::cout << "write amplification: " << m.write_amplification << '\n';
    for (size_t i = 0; i < m.num_files_per_level.size(); ++i) {
        if (m.num_files_per_level[i] > 0) {
            std::cout << "  L" << i << ": " << m.num_files_per_level[i] << " files, "
                      << m.bytes_per_level[i] << " bytes\n";
        }
    }

    std::cout << "\n=== Close & Recovery ===\n";

    db->close();
    std::cout << "database closed\n";

    auto db2 = BasonLevel::open({
        .dir = db_dir,
        .memtable_size = size_t{4} * 1024,
        .l0_compaction_trigger = 4,
        .level_size_base = size_t{64} * 1024,
        .max_levels = 7,
        .block_size = 512,
    });
    std::cout << "database reopened\n";

    result = db2->get("a");
    assert(result.has_value());
    std::cout << "get(a) -> " << result->value << "\n";

    result = db2->get("b");
    assert(!result.has_value());
    std::cout << "get(b) after recovery -> not found (was deleted)\n";

    result = db2->get("key:1050");
    assert(result.has_value());
    std::cout << "get(key:1050) after recovery -> found\n";

    db2->close();

    std::filesystem::remove_all(db_dir);

    return 0;
}
