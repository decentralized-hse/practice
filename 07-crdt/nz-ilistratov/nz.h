#pragma once

#include <stdint.h>

#include "types.h"
#include "utils.h"

struct URecord {
  uint64_t val;
  uint64_t src;
};

struct NRecord {
  struct URecord* data;
  uint32_t size;
};

bool Nvalid(const struct Bytes tlv);

char* Nstring(const struct Bytes tlv);

struct Bytes Nparse(const char* txt);

struct Bytes Ntlv(struct NRecord record);

uint64_t Nnative(const struct Bytes tlv);

struct Bytes Ndelta(const struct Bytes old_tlv, struct NRecord new_vals);

const struct Bytes Nmerge(const struct Bytes tlvs[], size_t tlvs_len);

struct IRecord {
  int64_t rev;
  uint64_t src;
  int64_t val;
};

struct ZRecord {
  struct IRecord* data;
  uint32_t size;
};

bool Zvalid(const struct Bytes tlv);

char* Zstring(const struct Bytes tlv);

struct Bytes Zparse(const char* txt);

struct Bytes Ztlv(struct ZRecord record);

int64_t Znative(const struct Bytes tlv);

struct Bytes Zdelta(const struct Bytes old_tlv, struct ZRecord new_vals);

const struct Bytes Zmerge(const struct Bytes tlvs[], size_t tlvs_len);
