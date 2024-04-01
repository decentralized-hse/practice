#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t ID;

struct Bytes {
  char* data;
  size_t len;
};

bool Equals(const struct Bytes lhs, const struct Bytes rhs);
