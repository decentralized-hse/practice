#include <cassert>
#include <cstddef>
#include <deque>
#include <utils/bytes.hpp>

namespace utils {

Bytes Bytes::Read(uint64_t count) {
  Bytes front(begin(), begin() + count);
  erase(begin(), begin() + count);
  return front;
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

void Bytes::WriteLittleEndian(uint64_t value, uint8_t min_bytes) {
  for (size_t i = 0; i < min_bytes || value > 0; ++i) {
    uint8_t least_byte = value & 0xff;
    push_back(least_byte);
    value >>= 8;
  }
}

void Bytes::Append(Bytes bytes) {
  insert(end(), bytes.begin(), bytes.end());
}

}  // namespace utils
