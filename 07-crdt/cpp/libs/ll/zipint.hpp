#include <cstdint>

#include <ll/bytes.hpp>

namespace ll {

Bytes Zip(uint64_t value);
uint64_t UnzipU64(const Bytes& bytes);

Bytes Zip(int64_t value);
int64_t UnzipI64(const Bytes& bytes);

Bytes Zip(double value);
double UnzipDouble(const Bytes& bytes);

///////////////////////////////////////////////

Bytes Zip(uint64_t big, uint64_t lil);
std::pair<uint64_t, uint64_t> UnzipU64Pair(const Bytes& bytes);

Bytes Zip(int64_t i, uint64_t u);
std::pair<int64_t, uint64_t> UnzipIU64Pair(const Bytes& bytes);

}  // namespace ll
