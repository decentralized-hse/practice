#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <ll/zipint.hpp>

namespace ll {

namespace {

uint8_t ByteLen(uint64_t value) {
  if (value == 0) {
    return 0;
  }

  if (value <= 0xff) {
    return 1;
  }

  if (value <= 0xffff) {
    return 2;
  }

  if (value <= 0xffffffff) {
    return 4;
  }

  return 8;
}

template <class To, class From>
To As(From value) {
  return *reinterpret_cast<To*>(&value);
}

uint64_t ZigZag(int64_t i) {
  return As<uint64_t>(i * 2) ^ As<uint64_t>(i >> 63);
}

int64_t ZagZig(uint64_t u) {
  uint64_t half = u >> 1;
  uint64_t mask = -(u & 1);
  return As<int64_t>(half ^ mask);
}

uint64_t Reverse(uint64_t n) {
  uint64_t rev = 0;
  for (int i = 0; i < 64; i++) {
    if (n & (1ULL << i))
      rev |= 1ULL << (63 - i);
  }
  return rev;
}

constexpr uint8_t SwitchPair(uint8_t l, uint8_t r) {
  return (l << 4) | r;
}

}  // namespace

Bytes Zip(uint64_t value) {
  Bytes bytes;
  bytes.WriteLittleEndian(value);
  return bytes;
}

uint64_t UnzipU64(const Bytes& bytes) {
  uint64_t value = 0;

  for (int i = int(bytes.size()) - 1; i >= 0; --i) {
    value <<= 8;
    value |= bytes[i];
  }

  return value;
}

Bytes Zip(int64_t value) {
  return Zip(ZigZag(value));
}

int64_t UnzipI64(const Bytes& bytes) {
  return ZagZig(UnzipU64(bytes));
}

Bytes Zip(double value) {
  uint64_t double_bytes = As<uint64_t>(value);
  return Zip(Reverse(double_bytes));
}

double UnzipDouble(const Bytes& bytes) {
  uint64_t double_bytes = UnzipU64(bytes);
  return As<double>(Reverse(double_bytes));
}

///////////////////////////////////////////////

Bytes Zip(uint64_t big, uint64_t lil) {
  Bytes bytes;

  uint8_t big_len = ByteLen(big);
  uint8_t lil_len = ByteLen(lil);

  switch (SwitchPair(big_len, lil_len)) {
    case SwitchPair(1, 0): {
      bytes.WriteLittleEndian(big, 1);
      return bytes;
    }
    case SwitchPair(0, 1):
    case SwitchPair(1, 1): {
      bytes.WriteLittleEndian(big, 1);
      bytes.WriteLittleEndian(lil, 1);
      return bytes;
    }
    case SwitchPair(2, 0):
    case SwitchPair(2, 1): {
      bytes.WriteLittleEndian(big, 2);
      bytes.WriteLittleEndian(lil, 1);
      return bytes;
    }
    case SwitchPair(0, 2):
    case SwitchPair(1, 2):
    case SwitchPair(2, 2): {
      bytes.WriteLittleEndian(big, 2);
      bytes.WriteLittleEndian(lil, 2);
      return bytes;
    }
    case SwitchPair(4, 0):
    case SwitchPair(4, 1): {
      bytes.WriteLittleEndian(big, 4);
      bytes.WriteLittleEndian(lil, 1);
      return bytes;
    }
    case SwitchPair(4, 2): {
      bytes.WriteLittleEndian(big, 4);
      bytes.WriteLittleEndian(lil, 2);
      return bytes;
    }
    case SwitchPair(0, 4):
    case SwitchPair(1, 4):
    case SwitchPair(2, 4):
    case SwitchPair(4, 4): {
      bytes.WriteLittleEndian(big, 4);
      bytes.WriteLittleEndian(lil, 4);
      return bytes;
    }
    case SwitchPair(8, 0):
    case SwitchPair(8, 1): {
      bytes.WriteLittleEndian(big, 8);
      bytes.WriteLittleEndian(lil, 1);
      return bytes;
    }
    case SwitchPair(8, 2): {
      bytes.WriteLittleEndian(big, 8);
      bytes.WriteLittleEndian(lil, 2);
      return bytes;
    }
    case SwitchPair(8, 4): {
      bytes.WriteLittleEndian(big, 8);
      bytes.WriteLittleEndian(lil, 4);
      return bytes;
    }
    case SwitchPair(0, 8):
    case SwitchPair(1, 8):
    case SwitchPair(2, 8):
    case SwitchPair(4, 8):
    case SwitchPair(8, 8): {
      bytes.WriteLittleEndian(big, 8);
      bytes.WriteLittleEndian(lil, 8);
      return bytes;
    }
  }

  return bytes;
}

std::pair<uint64_t, uint64_t> UnzipU64Pair(const Bytes& bytes) {
  uint64_t big = 0;
  uint64_t lil = 0;

  Bytes bytes_copy = bytes;

  switch (bytes.size()) {
    case 0: {
      big = lil = 0;
      return {big, lil};
    }
    case 1: {
      big = bytes_copy.ReadLittleEndian(1);
      lil = 0;
      return {big, lil};
    }
    case 2: {
      big = bytes_copy.ReadLittleEndian(1);
      lil = bytes_copy.ReadLittleEndian(1);
      return {big, lil};
    }
    case 3: {
      big = bytes_copy.ReadLittleEndian(2);
      lil = bytes_copy.ReadLittleEndian(1);
      return {big, lil};
    }
    case 4: {
      big = bytes_copy.ReadLittleEndian(2);
      lil = bytes_copy.ReadLittleEndian(2);
      return {big, lil};
    }
    case 5: {
      big = bytes_copy.ReadLittleEndian(4);
      lil = bytes_copy.ReadLittleEndian(1);
      return {big, lil};
    }
    case 6: {
      big = bytes_copy.ReadLittleEndian(4);
      lil = bytes_copy.ReadLittleEndian(2);
      return {big, lil};
    }
    case 8: {
      big = bytes_copy.ReadLittleEndian(4);
      lil = bytes_copy.ReadLittleEndian(4);
      return {big, lil};
    }
    case 9: {
      big = bytes_copy.ReadLittleEndian(8);
      lil = bytes_copy.ReadLittleEndian(1);
      return {big, lil};
    }
    case 10: {
      big = bytes_copy.ReadLittleEndian(8);
      lil = bytes_copy.ReadLittleEndian(2);
      return {big, lil};
    }
    case 12: {
      big = bytes_copy.ReadLittleEndian(8);
      lil = bytes_copy.ReadLittleEndian(4);
      return {big, lil};
    }
    case 16: {
      big = bytes_copy.ReadLittleEndian(8);
      lil = bytes_copy.ReadLittleEndian(8);
      return {big, lil};
    }
  }

  assert(bytes_copy.size() == 0);

  return {big, lil};
}

Bytes Zip(int64_t i, uint64_t u) {
  return Zip(ZigZag(i), u);
}

std::pair<int64_t, uint64_t> UnzipIU64Pair(const Bytes& bytes) {
  auto [z, u] = UnzipU64Pair(bytes);
  return {ZagZig(z), u};
}

}  // namespace ll
