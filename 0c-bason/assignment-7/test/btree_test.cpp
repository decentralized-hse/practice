#include "gtest/gtest.h"
#include "../src/btree.hpp"
#include "../src/page_manager.hpp"
#include "../src/mem_utils.hpp"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace basonlite;

namespace {

struct ExpectedRecord {
    codec::BasonType type;
    std::string value;
};

void cleanup_db_files(const std::string& db_path) {
    std::remove(db_path.c_str());
    std::remove((db_path + ".wal").c_str());
}

std::string make_key(std::size_t i) {
    std::ostringstream oss;
    oss << "key_" << std::setw(6) << std::setfill('0') << i;
    return oss.str();
}

codec::BasonRecord make_number_record(const std::string& value) {
    codec::BasonRecord rec;
    rec.type = codec::BasonType::Number;
    rec.value = value;
    return rec;
}

codec::BasonRecord make_object_record(const std::string& value) {
    codec::BasonRecord rec;
    rec.type = codec::BasonType::Object;
    rec.value = value;
    return rec;
}

codec::BasonRecord make_mixed_record(std::size_t key_index, std::size_t version) {
    if (((key_index + version) & 1u) == 0u) {
        return make_number_record(std::to_string(key_index * 7919u + version));
    }

    std::string payload((key_index % 8u) + 4u, static_cast<char>('a' + (version % 26u)));
    payload += "_";
    payload += std::to_string(version);
    return make_object_record(payload);
}

std::uint32_t freelist_next_page_no(const Page& page) {
    return utils::read_u32_ptr(page.bytes.data() + 1);
}

std::size_t leaf_used_bytes_for_test(const Page& page) {
    const auto records = BTree::decode_leaf(page);
    std::size_t used = 12 + records.size() * 2;

    for (const auto& r : records) {
        const auto payload = codec::bason_encode(r);
        used += 10 + payload.size();
    }

    return used;
}

std::size_t internal_used_bytes_for_test(const Page& page) {
    const auto node = BTree::decode_internal(page);
    std::size_t used = 12 + node.keys.size() * 2;

    for (std::size_t i = 0; i < node.keys.size(); ++i) {
        codec::BasonRecord cell;
        cell.type = codec::BasonType::Number;
        cell.key = node.keys[i];

        std::vector<std::uint8_t> buf(4);
        write_u32le(buf, 0, node.children[i]);
        cell.value.assign(reinterpret_cast<const char*>(buf.data()), buf.size());

        const auto payload = codec::bason_encode(cell);
        used += 10 + payload.size();
    }

    return used;
}

void check_leaf_order(const BTree& tree, PageManager& pm, std::uint32_t page_no) {
    Page page = pm.read_page(page_no);
    if (page_type(page) == static_cast<uint8_t>(PageType::Leaf)) {
        auto records = BTree::decode_leaf(page);
        for (std::size_t i = 1; i < records.size(); ++i) {
            EXPECT_LE(records[i - 1].key, records[i].key);
        }
        return;
    }

    if (page_type(page) == static_cast<uint8_t>(PageType::Interior)) {
        auto node = BTree::decode_internal(page);
        for (auto child : node.children) {
            check_leaf_order(tree, pm, child);
        }
    }
}

int leaf_depth(PageManager& pm, std::uint32_t page_no) {
    Page page = pm.read_page(page_no);
    if (page_type(page) == static_cast<uint8_t>(PageType::Leaf)) {
        return 0;
    }

    auto node = BTree::decode_internal(page);
    EXPECT_FALSE(node.children.empty());
    if (node.children.empty()) return 0;

    int depth = -1;
    for (auto child : node.children) {
        int d = leaf_depth(pm, child);
        if (depth == -1) depth = d;
        EXPECT_EQ(depth, d);
    }
    return depth + 1;
}

std::string first_key_in_subtree(PageManager& pm, std::uint32_t page_no) {
    Page page = pm.read_page(page_no);
    if (page_type(page) == static_cast<uint8_t>(PageType::Leaf)) {
        auto records = BTree::decode_leaf(page);
        return records.empty() ? std::string{} : records[0].key;
    }

    auto node = BTree::decode_internal(page);
    EXPECT_FALSE(node.children.empty());
    if (node.children.empty()) return "";
    return first_key_in_subtree(pm, node.children[0]);
}

void check_separator_keys(PageManager& pm, std::uint32_t page_no) {
    Page page = pm.read_page(page_no);
    if (page_type(page) != static_cast<uint8_t>(PageType::Interior)) {
        return;
    }

    auto node = BTree::decode_internal(page);
    for (std::size_t i = 0; i + 1 < node.children.size(); ++i) {
        std::string first = first_key_in_subtree(pm, node.children[i + 1]);
        EXPECT_EQ(first, node.keys[i]);
    }

    for (auto child : node.children) {
        check_separator_keys(pm, child);
    }
}

void check_page_reachability(PageManager& pm, std::uint32_t page_no, std::set<uint32_t>& visited) {
    if (page_no == 0 || visited.count(page_no)) {
        return;
    }

    visited.insert(page_no);
    Page page = pm.read_page(page_no);
    if (page_type(page) == static_cast<uint8_t>(PageType::Interior)) {
        auto node = BTree::decode_internal(page);
        for (auto child : node.children) {
            check_page_reachability(pm, child, visited);
        }
    }
}

void collect_tree_pages(PageManager& pm, std::uint32_t root_page, std::set<uint32_t>& visited) {
    if (root_page == 0 || visited.count(root_page)) {
        return;
    }

    visited.insert(root_page);
    Page page = pm.read_page(root_page);
    if (page_type(page) == static_cast<uint8_t>(PageType::Interior)) {
        auto node = BTree::decode_internal(page);
        for (auto child : node.children) {
            collect_tree_pages(pm, child, visited);
        }
    }
}

void collect_freelist_pages(PageManager& pm, std::uint32_t free_head, std::set<uint32_t>& free_pages) {
    std::unordered_set<std::uint32_t> seen;
    std::uint32_t cur = free_head;

    while (cur != 0) {
        ASSERT_TRUE(seen.insert(cur).second) << "cycle in freelist at page " << cur;
        free_pages.insert(cur);

        Page page = pm.read_page(cur);
        ASSERT_EQ(page_type(page), static_cast<uint8_t>(PageType::Freelist)) << "page " << cur << " is not a freelist page";

        cur = freelist_next_page_no(page);
    }
}

void check_no_page_leaks(PageManager& pm) {
    const auto h = pm.header();
    std::set<std::uint32_t> tree_pages;
    std::set<std::uint32_t> free_pages;

    const std::uint32_t root = pm.root_page();
    if (root != 0) {
        collect_tree_pages(pm, root, tree_pages);
    }

    collect_freelist_pages(pm, h.free_head, free_pages);

    for (std::uint32_t page_no = 2; page_no <= h.total_pages; ++page_no) {
        EXPECT_TRUE(tree_pages.count(page_no) || free_pages.count(page_no))
                << "page leak: page " << page_no << " is neither reachable nor freelist";
    }

    for (auto page_no : tree_pages) {
        EXPECT_FALSE(free_pages.count(page_no))
                << "page " << page_no << " is both reachable and freelist";
    }
}

void verify_subtree_partition(
    PageManager& pm,
    std::uint32_t page_no,
    std::uint32_t root_page_no,
    const std::optional<std::string>& lower = std::nullopt,
    const std::optional<std::string>& upper = std::nullopt
) {
    Page page = pm.read_page(page_no);
    const bool is_root = (page_no == root_page_no);
    const auto type = static_cast<PageType>(page_type(page));

    if (type == PageType::Leaf) {
        auto records = BTree::decode_leaf(page);
        EXPECT_LE(leaf_used_bytes_for_test(page), pm.page_size());

        if (!is_root) {
            EXPECT_FALSE(records.empty());
            EXPECT_GE(leaf_used_bytes_for_test(page), pm.page_size() / 4);
        }

        EXPECT_TRUE(std::is_sorted(
                records.begin(), records.end(),
                [](const codec::BasonRecord& a, const codec::BasonRecord& b) {
                    return a.key < b.key;
                }
        ));

        for (const auto& r : records) {
            if (lower) {
                EXPECT_GE(r.key, *lower);
            }
            if (upper) {
                EXPECT_LT(r.key, *upper);
            }
        }
        return;
    }

    ASSERT_EQ(type, PageType::Interior);
    auto node = BTree::decode_internal(page);

    EXPECT_EQ(node.children.size(), node.keys.size() + 1);
    EXPECT_TRUE(std::is_sorted(node.keys.begin(), node.keys.end()));
    EXPECT_LE(internal_used_bytes_for_test(page), pm.page_size());

    if (!is_root) {
        EXPECT_GE(node.children.size(), 2u);
        EXPECT_GE(internal_used_bytes_for_test(page), pm.page_size() / 4);
    } else {
        EXPECT_FALSE(node.children.empty());
    }

    for (std::size_t i = 0; i < node.children.size(); ++i) {
        std::optional<std::string> child_lower = lower;
        std::optional<std::string> child_upper = upper;

        if (i > 0) {
            child_lower = node.keys[i - 1];
        }
        if (i < node.keys.size()) {
            child_upper = node.keys[i];
        }

        verify_subtree_partition(pm, node.children[i], root_page_no, child_lower, child_upper);
    }
}

void check_tree_structure(PageManager& pm) {
    const std::uint32_t root = pm.root_page();
    ASSERT_NE(root, 0u);
    verify_subtree_partition(pm, root, root);
    EXPECT_GE(leaf_depth(pm, root), 0);
    check_separator_keys(pm, root);
}

void check_expected_contents(
    BTree& tree,
    const std::map<std::string, ExpectedRecord>& expected
) {
    for (const auto& [key, exp] : expected) {
        auto got = tree.find(key);
        ASSERT_TRUE(got.has_value()) << "missing key: " << key;
        EXPECT_EQ(got->key, key);
        EXPECT_EQ(got->type, exp.type);
        EXPECT_EQ(got->value, exp.value);
        EXPECT_TRUE(got->children.empty());
    }
}

void check_expected_absence(BTree& tree, const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        EXPECT_FALSE(tree.find(key).has_value()) << "deleted key still present: " << key;
    }
}

void check_dense_keyspace_against_expected(
    BTree& tree,
    const std::map<std::string, ExpectedRecord>& expected,
    std::size_t key_space
) {
    for (std::size_t i = 0; i < key_space; ++i) {
        const auto key = make_key(i);
        const auto it = expected.find(key);
        const auto got = tree.find(key);

        if (it == expected.end()) {
            EXPECT_FALSE(got.has_value()) << "unexpected key present: " << key;
            continue;
        }

        ASSERT_TRUE(got.has_value()) << "missing key: " << key;
        EXPECT_EQ(got->key, key);
        EXPECT_EQ(got->type, it->second.type);
        EXPECT_EQ(got->value, it->second.value);
        EXPECT_TRUE(got->children.empty());
    }
}

} // namespace

class BTreeTest : public ::testing::Test {
protected:
    void prepare_db(const std::string& path) {
        db_path_ = path;
        cleanup_db_files(db_path_);
    }

    void TearDown() override {
        cleanup_db_files(db_path_);
    }

    std::string db_path_;
};

TEST_F(BTreeTest, Insert) {
    prepare_db("btree_insert_maintains_structure.db");
    auto pm = PageManager::open(db_path_, 256);
    BTree tree(pm, 0);

    std::map<std::string, ExpectedRecord> expected;

    const std::size_t N = 1000;
    for (std::size_t i = 0; i < N; ++i) {
    const auto key = make_key(i);
    auto rec = make_mixed_record(i, 0);
    tree.insert(key, rec);
        expected[key] = {rec.type, rec.value};
    }

    check_tree_structure(pm);
    check_no_page_leaks(pm);
    check_leaf_order(tree, pm, pm.root_page());
    check_expected_contents(tree, expected);
}

TEST_F(BTreeTest, Find) {
    prepare_db("btree_find_mixed_types.db");
    auto pm = PageManager::open(db_path_, 256);
    BTree tree(pm, 0);

    std::map<std::string, ExpectedRecord> expected;

    const std::size_t N = 300;
    for (std::size_t i = 0; i < N; ++i) {
        const auto key = make_key(i);
        auto rec = make_mixed_record(i, 1);
        tree.insert(key, rec);
        expected[key] = {rec.type, rec.value};
    }

    for (std::size_t i = 0; i < N; ++i) {
        const auto key = make_key(i);
        auto got = tree.find(key);
        ASSERT_TRUE(got.has_value());
        EXPECT_EQ(got->key, key);
        EXPECT_EQ(got->type, expected[key].type);
        EXPECT_EQ(got->value, expected[key].value);
        EXPECT_TRUE(got->children.empty());
    }

    EXPECT_FALSE(tree.find("key_999999").has_value());
    EXPECT_FALSE(tree.find("absent_key").has_value());
}

TEST_F(BTreeTest, Remove) {
    prepare_db("btree_remove_rebalance.db");
    auto pm = PageManager::open(db_path_, 256);
    BTree tree(pm, 0);

    std::map<std::string, ExpectedRecord> expected;

    const std::size_t N = 10000;
    for (std::size_t i = 0; i < N; ++i) {
        const auto key = make_key(i);
        auto rec = make_mixed_record(i, 2);
        tree.insert(key, rec);
        expected[key] = {rec.type, rec.value};
    }

    std::vector<std::string> removed_keys;
    for (std::size_t i = 0; i < N; ++i) {
        if (i % 2 == 0 || i % 5 == 0) {
            const auto key = make_key(i);
            EXPECT_TRUE(tree.remove(key));
            expected.erase(key);
            removed_keys.push_back(key);
        }
    }

    check_tree_structure(pm);
    check_no_page_leaks(pm);
    check_leaf_order(tree, pm, pm.root_page());
    check_expected_contents(tree, expected);
    check_expected_absence(tree, removed_keys);
}

TEST_F(BTreeTest, NoPageLeaks) {
    prepare_db("btree_no_page_leaks.db");
    auto pm = PageManager::open(db_path_, 128);
    BTree tree(pm, 0);

    const std::size_t N = 1200;
    for (std::size_t i = 0; i < N; ++i) {
        const auto key = make_key(i);
        auto rec = make_number_record(std::to_string(i));
        tree.insert(key, rec);
    }

    for (std::size_t i = 0; i < N; ++i) {
        if (i % 3 == 0) {
            tree.remove(make_key(i));
        }
    }

    check_tree_structure(pm);
    check_no_page_leaks(pm);

    const auto h = pm.header();
    std::set<std::uint32_t> visited;
    if (pm.root_page() != 0) {
        check_page_reachability(pm, pm.root_page(), visited);
    }
    EXPECT_LE(visited.size(), h.total_pages);
    EXPECT_GT(h.total_pages, 1u);
}

TEST_F(BTreeTest, OperationStressPreservesDataAndStructure) {
    prepare_db("btree_mixed_stress.db");
    auto pm = PageManager::open(db_path_, 256);
    BTree tree(pm, 0);

    const std::size_t key_space = 3000;
    const std::size_t operations = 10000;
    std::map<std::string, ExpectedRecord> expected;

    std::mt19937 rng(42);
    std::uniform_int_distribution<std::size_t> key_dist(0, key_space - 1);
    std::uniform_int_distribution<int> op_dist(0, 99);

    for (std::size_t op = 0; op < operations; ++op) {
        const std::size_t k = key_dist(rng);
        const auto key = make_key(k);
        const int choice = op_dist(rng);

        if (choice < 62 || expected.empty()) {
            auto rec = make_mixed_record(k, op);
            tree.insert(key, rec);
            expected[key] = {rec.type, rec.value};
        } else {
            if (expected.erase(key) > 0) {
                EXPECT_TRUE(tree.remove(key));
            } else {
                EXPECT_FALSE(tree.remove(key));
            }
        }

        if ((op + 1) % 2500 == 0) {
            check_tree_structure(pm);
            check_no_page_leaks(pm);
        }
    }

    check_tree_structure(pm);
    check_no_page_leaks(pm);
    check_expected_contents(tree, expected);
    check_dense_keyspace_against_expected(tree, expected, key_space);
}

TEST_F(BTreeTest, DepthsBalanced) {
    prepare_db("btree_depth_stress.db");
    auto pm = PageManager::open(db_path_, 128);
    BTree tree(pm, 0);

    const std::size_t N = 6000;
    for (std::size_t i = 0; i < N; ++i) {
        const auto key = make_key(i);
        auto rec = make_number_record(std::to_string(i));
            tree.insert(key, rec);
        }

        for (std::size_t i = 0; i < N; ++i) {
            if (i % 4 == 0) {
            tree.remove(make_key(i));
        }
    }

    int depth = leaf_depth(pm, pm.root_page());
    EXPECT_GT(depth, 1);

    check_tree_structure(pm);
    check_no_page_leaks(pm);
}
