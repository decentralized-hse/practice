#include "types.h"
#include "lib/zipint_tlv.h"

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

struct RecordHeader ProbeHeader2(const struct Bytes tlv) {
  const struct ProbeHeader_return hdr = ProbeHeader(tlv.data, tlv.len);
  const struct RecordHeader res = {
    .lit = hdr.r0,
    .hdrlen = hdr.r1,
    .bodylen = hdr.r2
  };
  return res;
}
