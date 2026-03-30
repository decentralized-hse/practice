#pragma once

#include "codec/bason_record.hpp"
#include "page_manager.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace basonlite {

class BTreeIterator {
public:
    BTreeIterator() = default;

    bool valid() const noexcept {
        return pm_ != nullptr &&
               index_ < records_.size() &&
               (end_key_.empty() || records_[index_].key < end_key_);
    }

    void next();
    const codec::BasonRecord& record() const;

private:
    friend class BTree;

    BTreeIterator(PageManager* pm,
                  std::vector<std::uint32_t> internal_path,
                  std::vector<std::uint32_t> child_path,
                  std::vector<codec::BasonRecord> records,
                  std::size_t index,
                  std::string end_key)
    : pm_(pm),
      internal_path_(std::move(internal_path)),
      child_path_(std::move(child_path)),
      records_(std::move(records)),
      index_(index),
      end_key_(std::move(end_key)) {}

    void invalidate();

    bool advance_to_next_leaf();

    bool descend_leftmost(std::uint32_t page_no);

    void normalize_start();

    PageManager* pm_ = nullptr;
    std::vector<std::uint32_t> internal_path_;
    std::vector<std::uint32_t> child_path_;
    std::vector<codec::BasonRecord> records_;
    std::size_t index_ = 0;
    std::string end_key_;
};

class BTree {
public:
    BTree(PageManager& pm, std::uint32_t root_page);

    void insert(const std::string& key, const codec::BasonRecord& value);
    std::optional<codec::BasonRecord> find(const std::string& key);
    bool remove(const std::string& key);
    BTreeIterator scan(const std::string& start = "", const std::string& end = "");

private:
    friend class BTreeIterator;

    struct InternalNode {
        std::vector<std::string> keys;
        std::vector<std::uint32_t> children;
    };

    struct SplitResult {
        std::string separator;
        std::uint32_t right_page;
    };

    PageManager& pm_;
    std::uint32_t root_page_;

public:
    static std::vector<codec::BasonRecord> decode_leaf(const Page& page);
    static InternalNode decode_internal(const Page& page);

    uint32_t root_page() const { return root_page_; }

private:
    void ensure_root_exists();

    std::optional<SplitResult> insert_rec(std::uint32_t page_no, const codec::BasonRecord& rec);
    std::optional<codec::BasonRecord> find_rec(std::uint32_t page_no, const std::string& key);

    struct DeleteResult {
        bool removed = false;
        bool underflow = false;
    };

    DeleteResult erase_rec(std::uint32_t page_no, const std::string& key, bool is_root);

    void encode_leaf(Page& page, const std::vector<codec::BasonRecord>& records) const;
    void encode_internal(Page& page, const InternalNode& node) const;

    std::size_t leaf_used_bytes(const std::vector<codec::BasonRecord>& records) const;
    std::size_t internal_used_bytes(const InternalNode& node) const;

    bool leaf_fits(const std::vector<codec::BasonRecord>& records) const;
    bool internal_fits(const InternalNode& node) const;

    bool leaf_underfull(const std::vector<codec::BasonRecord>& records, bool is_root) const;
    bool internal_underfull(const InternalNode& node, bool is_root) const;

    std::size_t choose_leaf_split_index(const std::vector<codec::BasonRecord>& records) const;
    std::size_t choose_internal_split_index(const InternalNode& node) const;

    static std::size_t child_index_for_key(const std::vector<std::string>& keys, const std::string& key);

    std::string first_key_in_subtree(std::uint32_t page_no);
    void rebuild_separators(InternalNode& node);

    void rebalance(std::uint32_t page_no);
    void rebalance(InternalNode& parent, std::size_t child_index);
    void collapse_root_if_needed();
};

} // namespace basonlite