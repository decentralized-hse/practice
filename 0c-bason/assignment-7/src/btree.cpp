#include "btree.hpp"
#include "mem_utils.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <utility>

namespace basonlite {
namespace {

constexpr std::size_t kCellHeaderSize  = 10;  // u32 total_len, u16 inline_len, u32 overflow_page

std::string u32_to_binary_string(std::uint32_t value) {
    std::vector<std::uint8_t> buf(4);
    write_u32le(buf, 0, value);
    return std::string(reinterpret_cast<const char*>(buf.data()), buf.size());
}

std::uint32_t binary_string_to_u32(const std::string& s) {
    if (s.size() != 4) {
        throw std::runtime_error("expected 4-byte binary string");
    }
    return utils::read_u32_ptr(reinterpret_cast<const uint8_t*>(s.data()));
}

codec::BasonRecord make_internal_cell(const std::string& separator, std::uint32_t child_page_no) {
    codec::BasonRecord cell;
    cell.type = codec::BasonType::Number;
    cell.key = separator;
    cell.value = u32_to_binary_string(child_page_no);
    return cell;
}

std::vector<std::uint8_t> read_cell_bytes(const Page& page, std::uint16_t ofs) {
    if (ofs + kCellHeaderSize > page.size()) {
        throw std::runtime_error("corrupted cell offset");
    }

    const std::uint32_t total_len = utils::read_u32_ptr(page.bytes.data() + ofs);
    const std::uint16_t inline_len = utils::read_u16_ptr(page.bytes.data() + ofs + 4);

    if (inline_len > total_len) {
        throw std::runtime_error("corrupted cell header");
    }
    if (ofs + kCellHeaderSize + inline_len > page.size()) {
        throw std::runtime_error("corrupted inline payload");
    }

    std::vector<std::uint8_t> out;
    out.reserve(total_len);

    out.insert(out.end(),
               page.bytes.begin() + static_cast<std::ptrdiff_t>(ofs + kCellHeaderSize),
               page.bytes.begin() + static_cast<std::ptrdiff_t>(ofs + kCellHeaderSize + inline_len));

    return out;
}

std::vector<codec::BasonRecord> decode_records_from_page(const Page& page) {
    std::vector<codec::BasonRecord> records;
    const std::uint16_t count = cell_count(page);
    records.reserve(count);

    for (std::uint16_t i = 0; i < count; ++i) {
        const std::uint16_t ofs = get_cell_pointer(page, i);
        const auto payload = read_cell_bytes(page, ofs);
        auto [rec, consumed] = codec::bason_decode(payload.data(), payload.size());
        (void)consumed;
        records.push_back(std::move(rec));
    }

    return records;
}

bool page_is_underfull(PageManager& pm, std::size_t used_bytes, bool is_root) {
    if (is_root) {
        return false;
    }
    return used_bytes < pm.page_size() / 4;
}

} // namespace


void BTreeIterator::next() {
    if (!pm_) {
        return;
    }

    if (index_ + 1 < records_.size()) {
        ++index_;
        if (!end_key_.empty() && records_[index_].key >= end_key_) {
            invalidate();
        }
        return;
    }

    while (advance_to_next_leaf()) {
        index_ = 0;
        if (records_.empty()) {
            continue;
        }
        if (!end_key_.empty() && records_[index_].key >= end_key_) {
            invalidate();
        }
        return;
    }

    invalidate();
}

void BTreeIterator::invalidate() {
    pm_ = nullptr;
    internal_path_.clear();
    child_path_.clear();
    records_.clear();
    index_ = 0;
    end_key_.clear();
}

bool BTreeIterator::advance_to_next_leaf() {
    while (!internal_path_.empty()) {
        const std::uint32_t parent_page = internal_path_.back();
        const std::size_t child_idx = child_path_.back();

        Page parent_img = pm_->read_page(parent_page);
        BTree::InternalNode parent = BTree::decode_internal(parent_img);

        if (child_idx + 1 < parent.children.size()) {
            child_path_.back() = child_idx + 1;
            return descend_leftmost(parent.children[child_idx + 1]);
        }

        internal_path_.pop_back();
        child_path_.pop_back();
    }

    return false;
}

bool BTreeIterator::descend_leftmost(std::uint32_t page_no) {
    std::uint32_t current = page_no;

    while (true) {
        Page page = pm_->read_page(current);
        const auto type = static_cast<PageType>(page_type(page));

        if (type == PageType::Leaf) {
            records_ = BTree::decode_leaf(page);
            index_ = 0;
            return true;
        }

        if (type != PageType::Interior) {
            throw std::runtime_error("scan: unexpected page type");
        }

        BTree::InternalNode node = BTree::decode_internal(page);
        if (node.children.empty()) {
            return false;
        }

        internal_path_.push_back(current);
        child_path_.push_back(0);
        current = node.children.front();
    }
}

void BTreeIterator::normalize_start() {
    if (!pm_) {
        return;
    }

    if (!records_.empty() && index_ < records_.size()) {
        if (!end_key_.empty() && records_[index_].key >= end_key_) {
            invalidate();
        }
        return;
    }

    while (advance_to_next_leaf()) {
        index_ = 0;
        if (!records_.empty()) {
            if (!end_key_.empty() && records_[0].key >= end_key_) {
                invalidate();
            }
            return;
        }
    }

    invalidate();
}

const codec::BasonRecord& BTreeIterator::record() const {
    if (!valid()) {
        throw std::runtime_error("BTreeIterator: invalid iterator");
    }
    return records_[index_];
}

BTree::BTree(PageManager& pm, std::uint32_t root_page)
    : pm_(pm)
    , root_page_(root_page ? root_page : pm_.root_page()) {
    ensure_root_exists();
}

void BTree::ensure_root_exists() {
    if (root_page_ != 0) {
        pm_.set_root_page(root_page_);
        return;
    }

    root_page_ = pm_.allocate_page();

    Page page(pm_.page_size());
    page.zero();
    set_page_type(page, PageType::Leaf);
    set_cell_count(page, 0);
    set_free_ofs(page, static_cast<std::uint16_t>(page.size()));
    set_frag(page, 0);
    set_right_child(page, 0);

    pm_.write_page(root_page_, page);
    pm_.set_root_page(root_page_);
}

std::size_t BTree::child_index_for_key(const std::vector<std::string>& keys, const std::string& key) {
    return static_cast<std::size_t>(
        std::upper_bound(keys.begin(), keys.end(), key) - keys.begin()
    );
}

std::vector<codec::BasonRecord> BTree::decode_leaf(const Page& page) {
    return decode_records_from_page(page);
}

BTree::InternalNode BTree::decode_internal(const Page& page) {
    const std::uint16_t count = cell_count(page);

    BTree::InternalNode node;
    node.keys.reserve(count);
    node.children.reserve(count + 1);

    for (std::uint16_t i = 0; i < count; ++i) {
        const std::uint16_t ofs = get_cell_pointer(page, i);
        const auto payload = read_cell_bytes(page, ofs);
        auto [rec, _] = codec::bason_decode(payload.data(), payload.size());

        if (rec.value.size() != 4) {
            throw std::runtime_error("internal node cell must store 4-byte child page nummber");
        }

        node.keys.push_back(rec.key);
        node.children.push_back(binary_string_to_u32(rec.value));
    }

    node.children.push_back(right_child(page));

    if (node.children.size() != node.keys.size() + 1) {
        throw std::runtime_error("invalid internal node layout");
    }

    return node;
}

void BTree::encode_leaf(Page& page, const std::vector<codec::BasonRecord>& records) const {
    page.zero();
    set_page_type(page, PageType::Leaf);
    set_cell_count(page, static_cast<std::uint16_t>(records.size()));
    set_frag(page, 0);
    set_right_child(page, 0);

    std::size_t ofs = page.size();
    for (std::size_t i = 0; i < records.size(); ++i) {
        auto payload = codec::bason_encode(records[i]);
        std::size_t cell_size = kCellHeaderSize + payload.size();

        if (ofs < cell_size) {
            throw std::runtime_error("encode_leaf: page overflow");
        }

        ofs -= cell_size;

        utils::write_u32_ptr(page.bytes.data() + ofs, static_cast<uint32_t>(payload.size())); // total_len
        utils::write_u16_ptr(page.bytes.data() + ofs + 4, static_cast<uint16_t>(payload.size())); // inline_len

        std::memcpy(page.bytes.data() + ofs + kCellHeaderSize, payload.data(), payload.size());

        set_cell_pointer(page, i, static_cast<std::uint16_t>(ofs));
    }

    set_free_ofs(page, static_cast<std::uint16_t>(ofs));
}

void BTree::encode_internal(Page& page, const InternalNode& node) const {
    if (node.children.size() != node.keys.size() + 1) {
        throw std::runtime_error("internal node children must be keys+1");
    }

    page.zero();
    set_page_type(page, PageType::Interior);
    set_cell_count(page, static_cast<std::uint16_t>(node.keys.size()));
    set_frag(page, 0);
    set_right_child(page, node.children.back());


    std::size_t bytes = kBTreePageHeaderSize;
    std::vector<std::vector<std::uint8_t>> encoded_cells;
    encoded_cells.reserve(node.keys.size());
    for (std::size_t i = 0; i < node.keys.size(); ++i) {
        const codec::BasonRecord rec = make_internal_cell(node.keys[i], node.children[i]);
        const auto payload = codec::bason_encode(rec);
        encoded_cells.push_back(std::move(payload));
        bytes += kCellHeaderSize + encoded_cells.back().size();
    }

    if (bytes > page.size()) {
        throw std::runtime_error("encode_internal: page overflow");
    }

    std::size_t ofs = page.size();
    for (std::size_t i = 0; i < encoded_cells.size(); ++i) {
        const auto& payload = encoded_cells[i];
        std::size_t cell_size = kCellHeaderSize + payload.size();

        if (ofs < cell_size) {
            throw std::runtime_error("encode_internal: page overflow");
        }

        ofs -= cell_size;

        utils::write_u32_ptr(page.bytes.data() + ofs, static_cast<uint32_t>(payload.size())); // total_len
        utils::write_u16_ptr(page.bytes.data() + ofs + 4, static_cast<uint16_t>(payload.size())); // inline_len

        std::memcpy(page.bytes.data() + ofs + kCellHeaderSize, payload.data(), payload.size());

        set_cell_pointer(page, i, static_cast<std::uint16_t>(ofs));
    }

    set_free_ofs(page, static_cast<std::uint16_t>(ofs));
}

std::size_t BTree::leaf_used_bytes(const std::vector<codec::BasonRecord>& records) const {
    std::size_t used = kBTreePageHeaderSize + records.size() * 2;
    for (const auto& r : records) {
        const auto payload = codec::bason_encode(r);
        used += kCellHeaderSize + payload.size();
    }
    return used;
}

std::size_t BTree::internal_used_bytes(const InternalNode& node) const {
    std::size_t used = kBTreePageHeaderSize + node.keys.size() * 2;
    for (std::size_t i = 0; i < node.keys.size(); ++i) {
        const codec::BasonRecord rec = make_internal_cell(node.keys[i], node.children[i]);
        const auto payload = codec::bason_encode(rec);
        used += kCellHeaderSize + payload.size();
    }
    return used;
}

bool BTree::leaf_fits(const std::vector<codec::BasonRecord>& records) const {
    return leaf_used_bytes(records) <= pm_.page_size();
}

bool BTree::internal_fits(const InternalNode& node) const {
    return internal_used_bytes(node) <= pm_.page_size();
}

bool BTree::leaf_underfull(const std::vector<codec::BasonRecord>& records, bool is_root) const {
    return page_is_underfull(pm_, leaf_used_bytes(records), is_root);
}

bool BTree::internal_underfull(const InternalNode& node, bool is_root) const {
    return page_is_underfull(pm_, internal_used_bytes(node), is_root) || (!is_root && node.children.size() < 2);
}

std::size_t BTree::choose_leaf_split_index(const std::vector<codec::BasonRecord>& records) const {
    if (records.size() < 2) {
        return 1;
    }

    const std::size_t total = leaf_used_bytes(records) - kBTreePageHeaderSize;
    const std::size_t target = total / 2;

    std::size_t sum = 0;
    for (std::size_t i = 0; i + 1 < records.size(); ++i) {
        const auto payload = codec::bason_encode(records[i]);
        sum += kCellHeaderSize + payload.size() + 2;
        if (sum >= target) {
            return i + 1;
        }
    }

    return records.size() / 2;
}

std::size_t BTree::choose_internal_split_index(const InternalNode& node) const {
    if (node.children.size() < 2) {
        return 1;
    }
    return node.children.size() / 2;
}

std::string BTree::first_key_in_subtree(std::uint32_t page_no) {
    Page page = pm_.read_page(page_no);

    if (page_type(page) == static_cast<std::uint8_t>(PageType::Leaf)) {
        if (cell_count(page) == 0) {
            return {};
        }
        const std::uint16_t ofs = get_cell_pointer(page, 0);
        const auto payload = read_cell_bytes(page, ofs);
        auto [rec, _] = codec::bason_decode(payload.data(), payload.size());
        return rec.key;
    }

    if (page_type(page) != static_cast<std::uint8_t>(PageType::Interior)) {
        throw std::runtime_error("first_key_in_subtree: unexpected page type");
    }

    InternalNode node = BTree::decode_internal(page);
    return first_key_in_subtree(node.children.front());
}

void BTree::rebuild_separators(InternalNode& node) {
    if (node.children.size() < 2) {
        node.keys.clear();
        return;
    }

    node.keys.clear();
    node.keys.reserve(node.children.size() - 1);

    for (std::size_t i = 1; i < node.children.size(); ++i) {
        node.keys.push_back(first_key_in_subtree(node.children[i]));
    }
}

std::optional<BTree::SplitResult> BTree::insert_rec(std::uint32_t page_no, const codec::BasonRecord& rec) {
    Page page = pm_.read_page(page_no);

    if (page_type(page) == static_cast<std::uint8_t>(PageType::Leaf)) {
        auto records = decode_leaf(page);

        auto it = std::lower_bound(
            records.begin(), records.end(), rec.key,
            [](const codec::BasonRecord& r, const std::string& k) { return r.key < k; }
        );

        if (it != records.end() && it->key == rec.key) {
            *it = rec;
        } else {
            records.insert(it, rec);
        }

        if (leaf_fits(records)) {
            Page new_page(pm_.page_size());
            encode_leaf(new_page, records);
            pm_.write_page(page_no, new_page);
            return std::nullopt;
        }

        const std::size_t split_idx = choose_leaf_split_index(records);
        std::vector<codec::BasonRecord> left(records.begin(), records.begin() + static_cast<std::ptrdiff_t>(split_idx));
        std::vector<codec::BasonRecord> right(records.begin() + static_cast<std::ptrdiff_t>(split_idx), records.end());

        if (left.empty() || right.empty()) {
            throw std::runtime_error("leaf split failed");
        }

        const std::uint32_t right_page = pm_.allocate_page();

        Page left_page(pm_.page_size());
        Page right_page_obj(pm_.page_size());
        encode_leaf(left_page, left);
        encode_leaf(right_page_obj, right);

        pm_.write_page(page_no, left_page);
        pm_.write_page(right_page, right_page_obj);

        SplitResult split;
        split.separator = right.front().key;
        split.right_page = right_page;
        return split;
    }

    if (page_type(page) != static_cast<std::uint8_t>(PageType::Interior)) {
        throw std::runtime_error("insert: unexpected page type");
    }

    InternalNode node = BTree::decode_internal(page);
    const std::size_t idx = child_index_for_key(node.keys, rec.key);

    auto child_split = insert_rec(node.children[idx], rec);
    if (child_split) {
        node.children.insert(node.children.begin() + static_cast<std::ptrdiff_t>(idx + 1), child_split->right_page);
    }

    rebuild_separators(node);

    if (internal_fits(node)) {
        Page new_page(pm_.page_size());
        encode_internal(new_page, node);
        pm_.write_page(page_no, new_page);
        return std::nullopt;
    }

    const std::size_t split_child_index = choose_internal_split_index(node);
    if (split_child_index == 0 || split_child_index >= node.children.size()) {
        throw std::runtime_error("internal split failed");
    }

    InternalNode left;
    InternalNode right;

    left.children.assign(node.children.begin(), node.children.begin() + static_cast<std::ptrdiff_t>(split_child_index));
    right.children.assign(node.children.begin() + static_cast<std::ptrdiff_t>(split_child_index), node.children.end());

    rebuild_separators(left);
    rebuild_separators(right);

    if (left.children.empty() || right.children.empty()) {
        throw std::runtime_error("internal split produced empty side");
    }

    const std::uint32_t right_page_no = pm_.allocate_page();

    Page left_page(pm_.page_size());
    Page right_page(pm_.page_size());
    encode_internal(left_page, left);
    encode_internal(right_page, right);

    pm_.write_page(page_no, left_page);
    pm_.write_page(right_page_no, right_page);

    SplitResult split;
    split.separator = first_key_in_subtree(right.children.front());
    split.right_page = right_page_no;
    return split;
}

void BTree::insert(const std::string& key, const codec::BasonRecord& value) {
    ensure_root_exists();

    codec::BasonRecord rec = value;
    rec.key = key;

    auto split = insert_rec(root_page_, rec);
    if (split) {
        const std::uint32_t new_root = pm_.allocate_page();

        InternalNode root;
        root.children.push_back(root_page_);
        root.children.push_back(split->right_page);
        rebuild_separators(root);

        Page page(pm_.page_size());
        encode_internal(page, root);
        pm_.write_page(new_root, page);

        root_page_ = new_root;
        pm_.set_root_page(root_page_);
    }
}

std::optional<codec::BasonRecord> BTree::find_rec(std::uint32_t page_no, const std::string& key) {
    Page page = pm_.read_page(page_no);

    if (page_type(page) == static_cast<std::uint8_t>(PageType::Leaf)) {
        auto records = decode_leaf(page);
        auto it = std::lower_bound(
                records.begin(), records.end(), key,
                [](const codec::BasonRecord& r, const std::string& k) { return r.key < k; }
        );
        if (it != records.end() && it->key == key) {
            return *it;
        }
        return std::nullopt;
    }

    if (page_type(page) != static_cast<std::uint8_t>(PageType::Interior)) {
        throw std::runtime_error("find: unexpected page type");
    }

    InternalNode node = BTree::decode_internal(page);
    const std::size_t idx = child_index_for_key(node.keys, key);
    return find_rec(node.children[idx], key);
}

std::optional<codec::BasonRecord> BTree::find(const std::string& key) {
    if (root_page_ == 0) {
        return std::nullopt;
    }
    return find_rec(root_page_, key);
}

void BTree::rebalance(InternalNode& parent, std::size_t child_index) {
    if (child_index >= parent.children.size()) {
        throw std::runtime_error("rebalance: invalid index");
    }

    const std::uint32_t child_page_no = parent.children[child_index];
    Page child_page = pm_.read_page(child_page_no);
    const auto child_type = static_cast<PageType>(page_type(child_page));

    const bool has_left = child_index > 0;
    const bool has_right = child_index + 1 < parent.children.size();

    auto try_borrow_from_left = [&]() -> bool {
        if (!has_left) return false;

        const std::uint32_t left_page_no = parent.children[child_index - 1];
        Page left_page = pm_.read_page(left_page_no);
        if (static_cast<PageType>(page_type(left_page)) != child_type) return false;

        if (child_type == PageType::Leaf) {
            auto left_records = decode_leaf(left_page);
            auto child_records = decode_leaf(child_page);
            if (left_records.empty()) return false;

            std::vector<codec::BasonRecord> left_after = left_records;
            std::vector<codec::BasonRecord> child_after = child_records;

            const size_t total = left_after.size() + child_after.size();
            const size_t target = std::max<std::size_t>(total >> 1, 1);

            child_after.insert(child_after.begin(),
                               left_after.end() - static_cast<std::ptrdiff_t>(left_after.size() - target),
                               left_after.end());
            left_after.erase(left_after.end() - static_cast<std::ptrdiff_t>(left_after.size() - target),
                             left_after.end());

            if (leaf_underfull(left_after, false)) return false;
            if (leaf_underfull(child_after, false)) return false;

            Page p_left(pm_.page_size());
            Page p_child(pm_.page_size());
            encode_leaf(p_left, left_after);
            encode_leaf(p_child, child_after);

            pm_.write_page(left_page_no, p_left);
            pm_.write_page(child_page_no, p_child);
            return true;
        }

        if (child_type == PageType::Interior) {
            InternalNode left = BTree::decode_internal(left_page);
            InternalNode child = BTree::decode_internal(child_page);
            if (left.children.size() <= 2) return false;

            InternalNode left_after = left;
            InternalNode child_after = child;

            const size_t total = left_after.children.size() + child_after.children.size();
            const size_t target = std::max<std::size_t>(total >> 1, 1);

            child_after.children.insert(
                    child_after.children.begin(),
                    left_after.children.end() - static_cast<std::ptrdiff_t>(left_after.children.size() - target),
                    left_after.children.end());
            left_after.children.erase(
                    left_after.children.end() - static_cast<std::ptrdiff_t>(left_after.children.size() - target),
                    left_after.children.end());

            rebuild_separators(left_after);
            rebuild_separators(child_after);

            if (internal_underfull(left_after, false)) return false;
            if (internal_underfull(child_after, false)) return false;

            Page p_left(pm_.page_size());
            Page p_child(pm_.page_size());
            encode_internal(p_left, left_after);
            encode_internal(p_child, child_after);

            pm_.write_page(left_page_no, p_left);
            pm_.write_page(child_page_no, p_child);
            return true;
        }

        return false;
    };

    auto try_borrow_from_right = [&]() -> bool {
        if (!has_right) return false;

        const std::uint32_t right_page_no = parent.children[child_index + 1];
        Page right_page = pm_.read_page(right_page_no);
        if (static_cast<PageType>(page_type(right_page)) != child_type) return false;

        if (child_type == PageType::Leaf) {
            auto child_records = decode_leaf(child_page);
            auto right_records = decode_leaf(right_page);
            if (right_records.empty()) return false;

            std::vector<codec::BasonRecord> child_after = child_records;
            std::vector<codec::BasonRecord> right_after = right_records;

            const size_t total = right_after.size() + child_after.size();
            const size_t target = std::max<std::size_t>(total >> 1, 1);

            child_after.insert(child_after.end(),
                               right_after.begin(),
                               right_after.begin() + static_cast<std::ptrdiff_t>(right_after.size() - target));
            right_after.erase(right_after.begin(),
                              right_after.begin() + static_cast<std::ptrdiff_t>(right_after.size() - target));

            if (leaf_underfull(child_after, false)) return false;
            if (leaf_underfull(right_after, false)) return false;


            Page p_child(pm_.page_size());
            Page p_right(pm_.page_size());
            encode_leaf(p_child, child_after);
            encode_leaf(p_right, right_after);

            pm_.write_page(child_page_no, p_child);
            pm_.write_page(right_page_no, p_right);
            return true;
        }

        if (child_type == PageType::Interior) {
            InternalNode child = BTree::decode_internal(child_page);
            InternalNode right = BTree::decode_internal(right_page);
            if (right.children.size() <= 2) return false;

            InternalNode child_after = child;
            InternalNode right_after = right;

            const size_t total = right_after.children.size() + child_after.children.size();
            const size_t target = std::max<std::size_t>(total >> 1, 1);

            child_after.children.insert(
                    child_after.children.end(),
                    right_after.children.begin(),
                    right_after.children.begin() + static_cast<std::ptrdiff_t>(right_after.children.size() - target));
            right_after.children.erase(
                    right_after.children.begin(),
                    right_after.children.begin() + static_cast<std::ptrdiff_t>(right_after.children.size() - target));

            rebuild_separators(child_after);
            rebuild_separators(right_after);

            if (internal_underfull(child_after, false)) return false;
            if (internal_underfull(right_after, false)) return false;

            Page p_child(pm_.page_size());
            Page p_right(pm_.page_size());
            encode_internal(p_child, child_after);
            encode_internal(p_right, right_after);

            pm_.write_page(child_page_no, p_child);
            pm_.write_page(right_page_no, p_right);
            return true;
        }

        return false;
    };

    auto try_merge_with_left = [&]() -> bool {
        if (!has_left) return false;

        const std::uint32_t left_page_no = parent.children[child_index - 1];
        Page left_page = pm_.read_page(left_page_no);
        if (static_cast<PageType>(page_type(left_page)) != child_type) return false;

        if (child_type == PageType::Leaf) {
            auto left_records = decode_leaf(left_page);
            auto child_records = decode_leaf(child_page);

            left_records.insert(left_records.end(), child_records.begin(), child_records.end());
            if (!leaf_fits(left_records)) return false;

            Page p_left(pm_.page_size());
            encode_leaf(p_left, left_records);
            pm_.write_page(left_page_no, p_left);
            pm_.free_page(child_page_no);

            parent.children.erase(parent.children.begin() + static_cast<std::ptrdiff_t>(child_index));
            rebuild_separators(parent);
            return true;
        }

        if (child_type == PageType::Interior) {
            InternalNode left = BTree::decode_internal(left_page);
            InternalNode child = BTree::decode_internal(child_page);

            left.children.insert(left.children.end(), child.children.begin(), child.children.end());
            rebuild_separators(left);

            if (!internal_fits(left)) return false;

            Page p_left(pm_.page_size());
            encode_internal(p_left, left);
            pm_.write_page(left_page_no, p_left);
            pm_.free_page(child_page_no);

            parent.children.erase(parent.children.begin() + static_cast<std::ptrdiff_t>(child_index));
            rebuild_separators(parent);
            return true;
        }

        return false;
    };

    auto try_merge_with_right = [&]() -> bool {
        if (!has_right) return false;

        const std::uint32_t right_page_no = parent.children[child_index + 1];
        Page right_page = pm_.read_page(right_page_no);
        if (static_cast<PageType>(page_type(right_page)) != child_type) return false;

        if (child_type == PageType::Leaf) {
            auto child_records = decode_leaf(child_page);
            auto right_records = decode_leaf(right_page);

            child_records.insert(child_records.end(), right_records.begin(), right_records.end());
            if (!leaf_fits(child_records)) return false;

            Page p_child(pm_.page_size());
            encode_leaf(p_child, child_records);
            pm_.write_page(child_page_no, p_child);
            pm_.free_page(right_page_no);

            parent.children.erase(parent.children.begin() + static_cast<std::ptrdiff_t>(child_index + 1));
            rebuild_separators(parent);
            return true;
        }

        if (child_type == PageType::Interior) {
            InternalNode child = BTree::decode_internal(child_page);
            InternalNode right = BTree::decode_internal(right_page);

            child.children.insert(child.children.end(), right.children.begin(), right.children.end());
            rebuild_separators(child);

            if (!internal_fits(child)) return false;

            Page p_child(pm_.page_size());
            encode_internal(p_child, child);
            pm_.write_page(child_page_no, p_child);
            pm_.free_page(right_page_no);

            parent.children.erase(parent.children.begin() + static_cast<std::ptrdiff_t>(child_index + 1));
            rebuild_separators(parent);
            return true;
        }

        return false;
    };

    if (try_borrow_from_left()) return;
    if (try_borrow_from_right()) return;
    if (try_merge_with_left()) return;
    if (try_merge_with_right()) return;

    throw std::runtime_error("rebalance underfull failed");
}

BTree::DeleteResult BTree::erase_rec(std::uint32_t page_no, const std::string& key, bool is_root) {
    Page page = pm_.read_page(page_no);

    if (page_type(page) == static_cast<std::uint8_t>(PageType::Leaf)) {
        auto records = decode_leaf(page);

        auto it = std::lower_bound(
                records.begin(), records.end(), key,
                [](const codec::BasonRecord& r, const std::string& k) { return r.key < k; }
        );

        if (it == records.end() || it->key != key) {
            return {false, false};
        }

        records.erase(it);

        if (records.empty() && !is_root) {
            pm_.free_page(page_no);
            return {true, true};
        }

        Page p(pm_.page_size());
        encode_leaf(p, records);
        pm_.write_page(page_no, p);

        return {true, leaf_underfull(records, is_root)};
    }

    if (page_type(page) != static_cast<std::uint8_t>(PageType::Interior)) {
        throw std::runtime_error("remove: unexpected page type");
    }

    InternalNode node = BTree::decode_internal(page);
    const std::size_t idx = child_index_for_key(node.keys, key);

    DeleteResult child_result = erase_rec(node.children[idx], key, false);
    if (!child_result.removed) {
        return {false, false};
    }

    if (child_result.underflow) {
        Page child_page = pm_.read_page(node.children[idx]);
        if (page_type(child_page) == static_cast<uint8_t>(PageType::Leaf) && decode_leaf(child_page).empty()) {
            pm_.free_page(node.children[idx]);
            node.children.erase(node.children.begin() + static_cast<std::ptrdiff_t>(idx));
        } else if (page_type(child_page) == static_cast<uint8_t>(PageType::Interior) &&
                   decode_internal(child_page).children.empty()) {
            pm_.free_page(node.children[idx]);
            node.children.erase(node.children.begin() + static_cast<std::ptrdiff_t>(idx));
        } else {
            rebalance(node, idx);
        }
    }

    rebuild_separators(node);

    Page p(pm_.page_size());
    encode_internal(p, node);
    pm_.write_page(page_no, p);

    return {true, internal_underfull(node, is_root)};
}

void BTree::collapse_root_if_needed() {
    if (root_page_ == 0) {
        return;
    }

    Page root = pm_.read_page(root_page_);

    if (page_type(root) == static_cast<std::uint8_t>(PageType::Leaf)) {
        pm_.set_root_page(root_page_);
        return;
    }

    InternalNode node = BTree::decode_internal(root);
    if (node.children.size() == 1) {
        const std::uint32_t child = node.children.front();
        pm_.free_page(root_page_);
        root_page_ = child;
        pm_.set_root_page(root_page_);
    }
}

bool BTree::remove(const std::string& key) {
    if (root_page_ == 0) {
        return false;
    }

    DeleteResult result = erase_rec(root_page_, key, true);
    if (!result.removed) {
        return false;
    }

    collapse_root_if_needed();
    return true;
}

BTreeIterator BTree::scan(const std::string& start, const std::string& end) {
    if (root_page_ == 0) {
        return {};
    }

    std::vector<std::uint32_t> internal_path;
    std::vector<std::uint32_t> child_path;

    std::uint32_t page_no = root_page_;

    while (true) {
        Page page = pm_.read_page(page_no);
        const auto type = static_cast<PageType>(page_type(page));

        if (type == PageType::Leaf) {
            auto records = decode_leaf(page);

            std::size_t pos = 0;
            if (!start.empty()) {
                pos = static_cast<std::size_t>(
                    std::lower_bound(
                            records.begin(), records.end(), start,
                            [](const codec::BasonRecord& r, const std::string& k) {
                                return r.key < k;
                            }
                    ) - records.begin()
                );
            }

            BTreeIterator it(&pm_, std::move(internal_path), std::move(child_path), std::move(records), pos, end);

            it.normalize_start();
            return it;
        }

        if (type != PageType::Interior) {
            throw std::runtime_error("scan: unexpected page type");
        }

        InternalNode node = BTree::decode_internal(page);
        if (node.children.empty()) {
            return {};
        }

        const std::size_t idx = start.empty() ? 0 : child_index_for_key(node.keys, start);

        internal_path.push_back(page_no);
        child_path.push_back(idx);
        page_no = node.children[idx];
    }
}

void BTree::rebalance(std::uint32_t page_no) {
    if (page_no == 0) {
        return;
    }
    Page page = pm_.read_page(page_no);
    if (page_type(page) != static_cast<std::uint8_t>(PageType::Interior)) {
        return;
    }

    InternalNode node = BTree::decode_internal(page);
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        std::uint32_t child_page_no = node.children[i];
        Page child_page = pm_.read_page(child_page_no);

        bool underfull = false;
        if (page_type(child_page) == static_cast<std::uint8_t>(PageType::Leaf)) {
            auto records = decode_leaf(child_page);
            underfull = leaf_underfull(records, false);
        } else if (page_type(child_page) == static_cast<std::uint8_t>(PageType::Interior)) {
            InternalNode child_node = BTree::decode_internal(child_page);
            underfull = internal_underfull(child_node, false);
        }

        if (underfull) {
            rebalance(node, i);

            page = pm_.read_page(page_no);
            node = BTree::decode_internal(page);
        }
    }

    Page p(pm_.page_size());
    encode_internal(p, node);
    pm_.write_page(page_no, p);
}

} // namespace basonlite