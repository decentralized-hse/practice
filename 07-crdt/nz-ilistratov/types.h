#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t ID;

struct Bytes {
  char* data;
  size_t len;
};

struct RecordHeader {
  char lit;
  int32_t hdrlen;
  int32_t bodylen;
};

bool Equals(const struct Bytes lhs, const struct Bytes rhs);
struct RecordHeader ProbeHeader2(const struct Bytes tlv);
