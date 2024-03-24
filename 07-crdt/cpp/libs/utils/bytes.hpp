#include <cstdint>
#include <deque>
#include <cstddef>

namespace utils {

class Bytes : public std::deque<uint8_t> {
 public:
  void WriteLittleEndian(uint64_t value, uint8_t min_bytes = 0);
  uint64_t ReadLittleEndian(uint8_t max_bytes = 8);

  void Skip(uint64_t count);
  std::pair<Bytes, Bytes> Split(uint64_t count) const;
};

}  // namespace utils
