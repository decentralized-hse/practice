#include "nz.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "lib/zipint_tlv.h"

#include "types.h"
#include "utils.h"

struct NRecord InvalidateNRecord(struct NRecord *rec) {
  free(rec->data);
  rec->size = 0;
  return *rec;
}

int URecCmp(const void* lhs, const void* rhs) {
  struct URecord* rec_l = (struct URecord*)lhs;
  struct URecord* rec_r = (struct URecord*)rhs;
  if (rec_l->src < rec_r->src) {
    return -1;
  }
  if (rec_l->src != rec_r->src) {
    return 1;
  }
  if (rec_l->val < rec_r->val) {
    return -1;
  }
  if (rec_l->val != rec_r->val) {
    return 1;
  }
  return 0;
}

struct NRecord NRecordFromBytes(const struct Bytes tlv) {
  uint32_t cap = 16;
  struct NRecord res = {
    .size = 0,
    .data = calloc(cap, sizeof(struct URecord)),
  };
  const struct RecordHeader n_hdr = ProbeHeader2(tlv);
  if (tolower(n_hdr.lit) != 'n' && !isdigit(n_hdr.lit)) {
    return InvalidateNRecord(&res);
  }
  for (int offset = n_hdr.hdrlen; offset != tlv.len;) {
    const struct Bytes u_tlv = {
      tlv.data + offset,
      tlv.len - offset
    };
    const struct RecordHeader u_hdr = ProbeHeader2(u_tlv);
    if (tolower(u_hdr.lit) != 'u' && !isdigit(n_hdr.lit)) {
      return InvalidateNRecord(&res);
    }
    int u_tlv_len = u_hdr.hdrlen + u_hdr.bodylen;
    if (offset + u_tlv_len > tlv.len) {
      return InvalidateNRecord(&res);
    }
    offset += u_tlv_len;
    struct UnzipUint64Pair_return u_rec = UnzipUint64Pair(u_tlv.data + u_hdr.hdrlen, u_hdr.bodylen);
    res.data[res.size].val = u_rec.r0;
    res.data[res.size].src = u_rec.r1;
    ++res.size;
    if (res.size == cap) {
      res.data = realloc(res.data, cap * 2 * sizeof(struct URecord));
    }
  }
  res.data = realloc(res.data, res.size * sizeof(struct URecord));
  qsort(res.data, res.size, sizeof(struct URecord), URecCmp);
  for (uint32_t rec_idx = 1; rec_idx < res.size; rec_idx++) {
    if (res.data[rec_idx - 1].src == res.data[rec_idx].src) {
      return InvalidateNRecord(&res);
    }
  }
  return res;
}

bool Nvalid(const struct Bytes tlv) {
  struct NRecord rec = NRecordFromBytes(tlv);
  bool res = rec.size > 0;
  InvalidateNRecord(&rec);
  return res;
}

char* Nstring(const struct Bytes tlv) {
  struct NRecord n_rec = NRecordFromBytes(tlv);
  char* buf = malloc(8 + 33 * n_rec.size + 1);
  uint32_t offset = 0;
  offset += sprintf(buf + offset, "%x", n_rec.size);
  for (uint32_t rec_idx = 0; rec_idx < n_rec.size; rec_idx++) {
    offset += sprintf(buf + offset, " %lx %lx", n_rec.data[rec_idx].val, n_rec.data[rec_idx].src);
  }
  buf = realloc(buf, offset + 1);
  buf[offset] = '\0';
  return buf;
}

uint32_t AdvanceOffset(const char* txt, uint32_t offset) {
  while (txt[offset] != '\0') {
    ++offset;
    if (txt[offset] == ' ') {
      ++offset;
      break;
    }
  }
  return offset;
}

struct NRecord NRecordFromString(const char* txt) {
  struct NRecord res;
  uint32_t offset = 0;
  sscanf(txt + offset, "%x", &res.size);
  res.data = calloc(res.size, sizeof(struct NRecord));
  offset = AdvanceOffset(txt, offset);
  for (uint32_t rec_idx = 0; rec_idx < res.size; rec_idx++) {
    sscanf(txt + offset, " %lx %lx", &res.data[rec_idx].val, &res.data[rec_idx].src);
    offset = AdvanceOffset(txt, offset);
    offset = AdvanceOffset(txt, offset);
  }
  return res;
}

struct Bytes Nparse(const char* txt) {
  struct NRecord nrec = NRecordFromString(txt);
  struct Bytes tlv = Ntlv(nrec);
  InvalidateNRecord(&nrec);
  return tlv;
}

static const char* time0 = "\0\0\0\0";

struct Bytes Ntlv(struct NRecord record) {
  char* u_records = malloc(record.size * 21);
  int offset = 0;
  for (uint32_t record_idx = 0; record_idx < record.size; record_idx++) {
    struct ZipUint64Pair_return u_tlv = ZipUint64Pair(record.data[record_idx].val, record.data[record_idx].src);
    struct Record_return u_rec = Record('u', NULL, 0, u_tlv.r0, u_tlv.r1);
    free(u_tlv.r0);
    memcpy(u_records + offset, u_rec.r0, u_rec.r1);
    offset += u_rec.r1;
    free(u_rec.r0);
  }
  struct Record_return n_rec = Record('n', NULL, 0, u_records, offset);
  free(u_records);
  struct Bytes ret = {
    .data = n_rec.r0,
    .len = n_rec.r1
  };
  return ret;
}

uint64_t Nnative(const struct Bytes tlv) {
  struct NRecord rec = NRecordFromBytes(tlv);
  uint64_t native_val = 0;
  for (uint32_t rec_idx = 0; rec_idx < rec.size; rec_idx++) {
    native_val += rec.data[rec_idx].val;
  }
  InvalidateNRecord(&rec);
  return native_val;
}

struct NRecord NRecordDelta(struct NRecord old, struct NRecord new) {
  uint32_t it_o = 0;
  struct NRecord res = {
    .size = 0,
    .data = calloc(new.size, sizeof(struct URecord))
  };
  for (uint32_t it_n = 0; it_n < new.size; it_n++) {
    while (it_o < old.size && old.data[it_o].src < new.data[it_n].src) {
      ++it_o;
    }
    if (it_o == old.size || old.data[it_o].src > new.data[it_n].src ||
        old.data[it_o].val < new.data[it_n].val) {
      res.data[res.size] = new.data[it_n];
      ++res.size;
    }
  }
  res.data = realloc(res.data, res.size * sizeof(struct URecord));
  return res;
}

struct Bytes Ndelta(const struct Bytes old_tlv, struct NRecord new_vals) {
  struct NRecord old = NRecordFromBytes(old_tlv);
  struct NRecord delta = NRecordDelta(old, new_vals);
  InvalidateNRecord(&old);
  struct Bytes res = Ntlv(delta);
  InvalidateNRecord(&delta);
  return res;
}

struct NRecord NRecordMerge(struct NRecord lhs, struct NRecord rhs) {
  uint32_t it_l = 0;
  uint32_t it_r = 0;
  uint32_t num_insp = 0;
  while (it_l < lhs.size && it_r < rhs.size) {
    if (lhs.data[it_l].src < rhs.data[it_r].src) {
      ++it_l;
      continue;
    }
    if (lhs.data[it_l].src > rhs.data[it_r].src) {
      ++it_r;
      continue;
    }
    ++num_insp;
    ++it_l;
    ++it_r;
  }
  struct NRecord result_rec = {
    .size = lhs.size + rhs.size - num_insp
  };
  result_rec.data = calloc(result_rec.size, sizeof(struct URecord));
  it_l = it_r = 0;
  uint32_t dst = 0;
  while (it_l < lhs.size && it_r < rhs.size) {
    if (lhs.data[it_l].src < rhs.data[it_r].src) {
      result_rec.data[dst] = lhs.data[it_l];
      ++it_l;
      ++dst;
      continue;
    }
    if (lhs.data[it_l].src > rhs.data[it_r].src) {
      result_rec.data[dst] = rhs.data[it_r];
      ++it_r;
      ++dst;
      continue;
    }
    result_rec.data[dst] = lhs.data[it_l];
    if (lhs.data[it_l].val < rhs.data[it_r].val) {
      result_rec.data[dst] = lhs.data[it_r];
    }
    ++it_l;
    ++it_r;
    ++dst;
  }
  if (it_l < lhs.size) {
    memcpy(result_rec.data + dst, lhs.data + it_l, (lhs.size - it_l) * sizeof(struct URecord));
  }
  if (it_r < rhs.size) {
    memcpy(result_rec.data + dst, rhs.data + it_r, (rhs.size - it_r) * sizeof(struct URecord));
  }
  return result_rec;
}

const struct Bytes Nmerge(const struct Bytes tlvs[], size_t tlvs_len) {
  if (tlvs_len == 1) {
    return tlvs[0];
  }
  struct NRecord res_n_rec = NRecordFromBytes(tlvs[0]);
  for (size_t tlv_idx = 1; tlv_idx < tlvs_len; tlv_idx++) {
    struct NRecord next_rec = NRecordFromBytes(tlvs[1]);
    struct NRecord merged_rec = NRecordMerge(res_n_rec, next_rec);
    InvalidateNRecord(&res_n_rec);
    InvalidateNRecord(&next_rec);
    res_n_rec = merged_rec;
  }
  struct Bytes res = Ntlv(res_n_rec);
  InvalidateNRecord(&res_n_rec);
  return res;
}

struct ZRecord InvalidateZRecord(struct ZRecord* rec) {
  free(rec->data);
  rec->size = 0;
  return *rec;
}

int IRecCmp(const void* lhs, const void* rhs) {
  const struct IRecord *rec_l = lhs;
  const struct IRecord *rec_r = rhs;
  if (rec_l->src < rec_r->src) {
    return -1;
  }
  if (rec_l->src > rec_r->src) {
    return 1;
  }
  if (rec_l->rev != rec_r->rev) {
    return rec_l->rev - rec_r->rev;
  }
  return rec_l->val - rec_r->val;
}

struct ZRecord ZRecordFromBytes(struct Bytes tlv) {
  uint32_t cap = 16;
  struct ZRecord res = {
    .data = calloc(cap, sizeof(struct URecord)),
    .size = 0
  };
  const struct RecordHeader z_hdr = ProbeHeader2(tlv);
  if (tolower(z_hdr.lit) != 'z') {
    return InvalidateZRecord(&res);
  }
  for (int offset = z_hdr.hdrlen; offset != tlv.len;) {
    const struct Bytes i_tlv =  {
      tlv.data + offset,
      tlv.len - offset
    };
    const struct RecordHeader i_hdr = ProbeHeader2(i_tlv);
    if (tolower(i_hdr.lit) != 'i') {
      return InvalidateZRecord(&res);
    }
    int i_tlv_len = i_hdr.hdrlen + i_hdr.bodylen;
    if (offset + i_tlv_len > tlv.len) {
      return InvalidateZRecord(&res);
    }
    const struct Bytes t_tlv = {
      tlv.data + offset + i_hdr.hdrlen,
      tlv.len - offset
    };
    const struct RecordHeader t_hdr = ProbeHeader2(t_tlv);
    struct UnzipIntUint64Pair_return rev_src = UnzipIntUint64Pair(tlv.data + offset + i_hdr.hdrlen + t_hdr.hdrlen, t_hdr.bodylen);
    int t_len = t_hdr.hdrlen + t_hdr.bodylen;
    int64_t val = UnzipInt64(tlv.data + offset + i_hdr.hdrlen + t_len, i_hdr.bodylen - t_len);
    struct IRecord i_rec = {
      .rev = rev_src.r0,
      .src = rev_src.r1,
      .val = val
    };
    offset += i_tlv_len;
    res.data[res.size] = i_rec;
    res.size += 1;
  }
  res.data = realloc(res.data, res.size & sizeof(struct IRecord));
  qsort(res.data, res.size, sizeof(struct IRecord), IRecCmp);
  for (uint32_t rec_idx = 1; rec_idx < res.size; rec_idx++) {
    if (res.data[rec_idx - 1].src == res.data[rec_idx].src) {
      return InvalidateZRecord(&res);
    }
  }
  return res;
}

bool Zvalid(const struct Bytes tlv) {
  struct ZRecord rec = ZRecordFromBytes(tlv);
  bool res = rec.size > 0;
  InvalidateZRecord(&rec);
  return res;
}

char* Zstring(const struct Bytes tlv) {
  struct ZRecord rec = ZRecordFromBytes(tlv);
  char* buf = malloc(8 + 51 * rec.size + 1);
  uint32_t offset = 0;
  offset += sprintf(buf + offset, "%x", rec.size);
  for (uint32_t rec_idx = 0; rec_idx < rec.size; rec_idx++) {
    offset += sprintf(buf + offset, " %lx %lx %lx", rec.data[rec_idx].rev, rec.data[rec_idx].src, rec.data[rec_idx].val);
  }
  InvalidateZRecord(&rec);
  buf[offset] = '\0';
  buf = realloc(buf, offset + 1);
  return buf;
}

struct ZRecord ZRecordFromString(const char* txt) {
  struct ZRecord res;
  uint32_t offset = 0;
  sscanf(txt + offset, "%x", &res.size);
  res.data = calloc(res.size, sizeof(struct IRecord));
  offset = AdvanceOffset(txt, offset);
  for (uint32_t rec_idx = 0; rec_idx < res.size; rec_idx++) {
    sscanf(txt + offset, " %lx %lx %lx", &res.data[rec_idx].rev, &res.data[rec_idx].src, &res.data[rec_idx].val);
    offset = AdvanceOffset(txt, offset);
    offset = AdvanceOffset(txt, offset);
    offset = AdvanceOffset(txt, offset);
  }
  return res;
}

struct Bytes Zparse(const char* txt) {
  struct ZRecord rec = ZRecordFromString(txt);
  struct Bytes res = Ztlv(rec);
  InvalidateZRecord(&rec);
  return res;
}

struct Bytes Ztlv(struct ZRecord rec) {
  char* i_records = malloc(9 + rec.size * (36));
  uint32_t offset = 0;
  for (uint32_t rec_idx = 0; rec_idx < rec.size; rec_idx++) {
    struct ZipIntUint64Pair_return rev_src = ZipIntUint64Pair(rec.data[rec_idx].rev, rec.data[rec_idx].src);
    struct Record_return t_rec = Record('t', NULL, 0, rev_src.r0, rev_src.r1);
    struct ZipInt64_return val = ZipInt64(rec.data[rec_idx].val);
    int i_body_len = t_rec.r1 + val.r1;
    assert(i_body_len <= 36);
    char i_body[36];
    memcpy(i_body, t_rec.r0, t_rec.r1);
    memcpy(i_body + t_rec.r1, val.r0, val.r1);
    free(t_rec.r0);
    free(val.r0);
    struct Record_return i_rec = Record('i', NULL, 0, i_body, i_body_len);
    memcpy(i_records + offset, i_rec.r0, i_rec.r1);
    offset += i_rec.r1;
    free(i_rec.r0);
  }
  struct Record_return z_rec = Record('z', NULL, 0, i_records, offset);
  free(i_records);
  struct Bytes ret = {
    .data = z_rec.r0,
    .len = z_rec.r1,
  };
  return ret;
}

int64_t Znative(const struct Bytes tlv) {
  struct ZRecord rec = ZRecordFromBytes(tlv);
  int64_t val = 0;
  for (uint32_t rec_idx = 0; rec_idx < rec.size; rec_idx++) {
    val += rec.data[rec_idx].val;
  }
  InvalidateZRecord(&rec);
  return val;
}

struct Bytes Zdelta(const struct Bytes old_tlv, struct ZRecord new_vals);

const struct Bytes Zmerge(const struct Bytes tlvs[], size_t tlvs_len);