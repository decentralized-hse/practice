#pragma once

#include <cstdint>
#include <deque>
#include <cstddef>

namespace ll {

class Bytes : public std::deque<uint8_t> {
 public:
  using std::deque<uint8_t>::deque;

  Bytes Read(uint64_t count);
  uint64_t ReadLittleEndian(uint8_t max_bytes = 8);

  void Append(Bytes bytes);
  void WriteLittleEndian(uint64_t value, uint8_t min_bytes = 0);
};

}  // namespace ll
