#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void PrintBytes(const char* name, const struct Bytes bytes) {
  printf("%s LEN: %ld, BYTES:\t0x", name, bytes.len);
  for (size_t i = 0; i < bytes.len; ++i) {
    printf("%02x", (uint8_t)(bytes.data[i]));
    if (i + 1 == bytes.len) {
      printf("\n");
    } else {
      printf(":");
    }
  }
}
void PrintStringBytes(const char* name, const char* txt) {
  const struct Bytes bytes = {.data = (char*)txt, .len = strlen(txt)};
  PrintBytes(name, bytes);
}

/// @note just `*(uint32_t*)data`
/// may not work under undefined sanitizer
/// because of incorrect alignment
uint32_t LittleEndianUint32(const char* data) {
  uint32_t ret = 0;
  for (size_t i = 0; i < 4; ++i) {
    ret |= ((uint32_t)data[i]) << (8 * i);
  }

  return ret;
}

/// @note just `*(uint32_t*)out = v`
/// may not work under undefined sanitizer
/// because of incorrect alignment
void LittleEndianPutUint32(char* out, uint32_t v) {
  for (size_t i = 0; i < 4; ++i) {
    out[i] = (char)(v >> (8 * i));
  }
}
