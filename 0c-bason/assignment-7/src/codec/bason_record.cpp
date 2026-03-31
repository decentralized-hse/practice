#include "bason_record.hpp"

#include <stdexcept>

namespace basonlite::codec {

static void write_u16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

static uint16_t read_u16(const uint8_t* data, size_t& pos, size_t len) {
    if (pos + 2 > len) {
        throw std::runtime_error("decode: out of bounds (u16)");
    }
    uint16_t v = data[pos] | (data[pos + 1] << 8);
    pos += 2;
    return v;
}

std::vector<uint8_t> bason_encode(const BasonRecord& r) {
    std::vector<uint8_t> out;

    // type
    out.push_back(static_cast<uint8_t>(r.type));

    // key
    write_u16(out, static_cast<uint16_t>(r.key.size()));
    out.insert(out.end(), r.key.begin(), r.key.end());

    // value
    write_u16(out, static_cast<uint16_t>(r.value.size()));
    out.insert(out.end(), r.value.begin(), r.value.end());

    // children count
    write_u16(out, static_cast<uint16_t>(r.children.size()));

    // children
    for (const auto& c : r.children) {
        auto child_bytes = bason_encode(c);

        write_u16(out, static_cast<uint16_t>(child_bytes.size()));
        out.insert(out.end(), child_bytes.begin(), child_bytes.end());
    }

    return out;
}

std::pair<BasonRecord, size_t> bason_decode(const uint8_t* data, size_t len) {
    if (len < 1) {
        throw std::runtime_error("decode: empty buffer");
    }
    size_t pos = 0;
    BasonRecord r;

    // type
    r.type = static_cast<BasonType>(data[pos++]);

    // key
    uint16_t key_len = read_u16(data, pos, len);
    if (pos + key_len > len) {
        throw std::runtime_error("decode: key overflow");
    }
    r.key = std::string(reinterpret_cast<const char*>(data + pos), key_len);
    pos += key_len;

    // value
    uint16_t val_len = read_u16(data, pos, len);
    if (pos + val_len > len) {
        throw std::runtime_error("decode: value overflow");
    }
    r.value = std::string(reinterpret_cast<const char*>(data + pos), val_len);
    pos += val_len;

    // children
    uint16_t child_count = read_u16(data, pos, len);

    r.children.reserve(child_count);

    for (uint16_t i = 0; i < child_count; ++i) {
        uint16_t child_size = read_u16(data, pos, len);

        if (pos + child_size > len) {
            throw std::runtime_error("decode: child overflow");
        }

        auto [child, consumed] = bason_decode(data + pos, child_size);

        (void)consumed;

        pos += child_size;
        r.children.push_back(child);
    }

    return {r, pos};
}

} // namespace basonlite::codec