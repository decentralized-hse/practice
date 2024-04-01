#include "types.h"

bool Equals(const struct Bytes lhs, const struct Bytes rhs) {
  if (lhs.len != rhs.len) {
    return false;
  }

  for (size_t i = 0; i < lhs.len; ++i) {
    if (lhs.data[i] != rhs.data[i]) {
      return false;
    }
  }

  return true;
}
