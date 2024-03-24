#include <cassert>
#include <cstddef>
#include <utils/bytes.hpp>

namespace utils {

void Bytes::WriteLittleEndian(uint64_t value, uint8_t min_bytes) {
  for (size_t i = 0; i < min_bytes || value > 0; ++i) {
    uint8_t least_byte = value & 0xff;
    push_back(least_byte);
    value >>= 8;
  }
}

uint64_t Bytes::ReadLittleEndian(uint8_t max_bytes) {
  assert(max_bytes <= size());

  uint64_t value = 0;

  for (size_t i = 0; i < max_bytes; ++i) {
    uint64_t byte = front();
    pop_front();
    value |= (byte << (8 * i));
  }

  return value;
}

void Bytes::Skip(uint64_t count) {
  assert(count <= size());

  for (size_t i = 0; i < count; ++i) {
    pop_front();
  }
}

std::pair<Bytes, Bytes> Bytes::Split(uint64_t count) const {
  Bytes l, r;

  for (size_t i = 0; i < size(); ++i) {
    if (i < count) {
      l.push_back(at(i));
    } else {
      r.push_back(at(i));
    }
  }

  return {std::move(l), std::move(r)};
}

}  // namespace utils
