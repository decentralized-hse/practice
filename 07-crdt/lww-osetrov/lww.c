#include "lww.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/zipint_tlv.h"

static const char* time0 = "\0\0\0\0";
static const char special_chars[] = {
    0x0,  0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0xb,
    0xc,  0xe,  0xf,  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};
static const char* hex = "0123456789abcdef";

struct Bytes LWWvalue(const struct Bytes tlv) {
  const struct ProbeHeader_return ret = ProbeHeader(tlv.data, tlv.len);
  int hdrlen = ret.r1;
  int bodylen = ret.r2;

  struct Bytes bytes = {.data = tlv.data + hdrlen + 4,
                        .len = (size_t)(bodylen - 4)};
  return bytes;
}

struct Bytes LWWdelta(const struct Bytes old_tlv, const struct Bytes new_tlv) {
  struct ProbeHeader_return ret1 = ProbeHeader(old_tlv.data, old_tlv.len);
  struct ProbeHeader_return ret2 = ProbeHeader(new_tlv.data, new_tlv.len);
  char lit = ret1.r0;
  int old_hdrlen = ret1.r1;
  int new_hdrlen = ret2.r1;

  uint32_t time = LittleEndianUint32(old_tlv.data + old_hdrlen);
  char new_time[4] = {0};
  LittleEndianPutUint32(new_time, time + 1);

  struct Record_return ret =
      Record(lit, new_time, 4, new_tlv.data + new_hdrlen + 4,
             new_tlv.len - (new_hdrlen + 4));
  struct Bytes bytes = {
      .data = ret.r0,
      .len = ret.r1,
  };

  return bytes;
}

const struct Bytes LWWmerge(char lit, const struct Bytes tlvs[],
                            size_t tlvs_len) {
  struct Bytes win;
  uint32_t max_time = 0;
  for (size_t i = 0; i < tlvs_len; ++i) {
    if (tlvs[i].len > 0) {
      struct ProbeHeader_return ret = ProbeHeader(tlvs[i].data, tlvs[i].len);
      int hdrlen = ret.r1;
      int bodylen = ret.r2;
      uint32_t time = LittleEndianUint32(tlvs[i].data + hdrlen);
      if (time > max_time) {
        max_time = time;
        win = tlvs[i];
      }
    }
  }

  return win;
}

bool Ivalid(const struct Bytes tlv) {
  return tlv.len >= 4;
}

int64_t Iplain(const struct Bytes tlv) {
  const struct Bytes zipped = LWWvalue(tlv);
  int64_t unzipped = UnzipInt64(zipped.data, zipped.len);
  return unzipped;
}

char* Istring(const struct Bytes tlv) {
  char* data = (char*)malloc(1024);
  sprintf(data, "%ld", Iplain(tlv));

  return data;
}

struct Bytes Iparse(const char* txt) {
  int64_t i;
  sscanf(txt, "%ld", &i);

  return Itlv(i);
}

struct Bytes Itlv(int64_t i) {
  struct ZipInt64_return zipped = ZipInt64(i);

  struct Record_return ret = Record('I', time0, 4, zipped.r0, zipped.r1);

  struct Bytes bytes = {.data = ret.r0, .len = (size_t)ret.r1};

  return bytes;
}

struct Bytes Idelta(const struct Bytes old_tlv, int64_t new_val) {
  const struct Bytes new_tlv = Itlv(new_val);

  return LWWdelta(old_tlv, new_tlv);
}

const struct Bytes Imerge(const struct Bytes tlvs[], size_t tlvs_len) {
  return LWWmerge('I', tlvs, tlvs_len);
}

bool Svalid(const struct Bytes tlv) {
  return tlv.len >= 4;
}

char* Sstring(const struct Bytes tlv) {
  const struct Bytes val = LWWvalue(tlv);

  char* buf = (char*)malloc(tlv.len * 6 + 3);
  size_t i = 0;
  buf[i++] = '"';
  for (size_t j = 0; j < val.len; ++j) {
    const char b = val.data[j];

    if (b == '\\' || b == '"') {
      buf[i++] = '\\';
      buf[i++] = b;
    } else if (b == '\n') {
      buf[i++] = '\\';
      buf[i++] = 'n';
    } else if (b == '\r') {
      buf[i++] = '\\';
      buf[i++] = 'r';
    } else if (b == '\t') {
      buf[i++] = '\\';
      buf[i++] = 't';
    } else if (memchr(special_chars, b, sizeof(special_chars))) {
      buf[i++] = '\\';
      buf[i++] = 'u';
      buf[i++] = '0';
      buf[i++] = '0';
      buf[i++] = hex[b >> 4];
      buf[i++] = hex[b & 0xF];
    } else {
      buf[i++] = b;
    }
  }
  buf[i++] = '"';
  buf[i++] = '\0';

  return buf;
}

struct Bytes Sparse(const char* txt) {
  const struct Unescape_return ret = Unescape(txt + 1, strlen(txt) - 2);

  struct Record_return record = Record('S', time0, 4, ret.r0, ret.r1);

  struct Bytes bytes = {.data = record.r0, .len = (size_t)record.r1};

  return bytes;
}

struct Bytes Stlv(const char* txt) {
  struct Record_return ret = Record('I', time0, 4, txt, strlen(txt));

  struct Bytes bytes = {.data = ret.r0, .len = (size_t)ret.r1};

  return bytes;
}

char* Splain(const struct Bytes tlv) {
  struct Bytes value = LWWvalue(tlv);

  return value.data;
}

struct Bytes Sdelta(const struct Bytes old_tlv, const char* new_txt) {
  const struct Bytes new_tlv = Stlv(new_txt);

  return LWWdelta(old_tlv, new_tlv);
}

const struct Bytes Smerge(const struct Bytes tlvs[], size_t tlvs_len) {
  return LWWmerge('S', tlvs, tlvs_len);
}

bool Fvalid(const struct Bytes tlv) {
  return tlv.len >= 4;
}

char* Fstring(const struct Bytes tlv) {
  char* data = (char*)malloc(1024);
  sprintf(data, "%lf", Fplain(tlv));

  return data;
}

struct Bytes Fparse(const char* txt) {
  double f;
  sscanf(txt, "%lf", &f);

  return Ftlv(f);
}

struct Bytes Ftlv(double f) {
  uint64_t bits = *(uint64_t*)&f;
  struct ZipUint64_return zipped = ZipUint64(bits);
  struct Record_return ret = Record('F', time0, 4, zipped.r0, zipped.r1);

  struct Bytes bytes = {.data = ret.r0, .len = (size_t)ret.r1};

  return bytes;
}

double Fplain(const struct Bytes tlv) {
  struct Bytes value = LWWvalue(tlv);
  uint64_t bits = UnzipUint64(value.data, value.len);

  return *(double*)&bits;
}

struct Bytes Fdelta(const struct Bytes old_tlv, double new_val) {
  const struct Bytes new_tlv = Ftlv(new_val);

  return LWWdelta(old_tlv, new_tlv);
}

const struct Bytes Fmerge(const struct Bytes tlvs[], size_t tlvs_len) {
  return LWWmerge('F', tlvs, tlvs_len);
}

bool Rvalid(const struct Bytes tlv) {
  return tlv.len >= 4;
}

char* Rstring(const struct Bytes tlv) {
  ID id = Rplain(tlv);

  struct SrcSeqOff_return src_seq_off = SrcSeqOff(id);
  uint32_t src = src_seq_off.r0;
  uint32_t seq = src_seq_off.r1;
  uint16_t off = src_seq_off.r2;

  char* buf = (char*)malloc(24);
  if (src == 0 && seq == 0) {
    sprintf(buf, "%x", off);
  } else if (off == 0) {
    sprintf(buf, "%x-%x", src, seq);
  } else {
    sprintf(buf, "%x-%x-%x", src, seq, off);
  }

  return buf;
}

struct Bytes Rparse(const char* txt) {
  ID parsed = ParseID(txt, strlen(txt));

  return Rtlv(parsed);
}

struct Bytes Rtlv(ID id) {
  struct ZipID_return zipped = ZipID(id);
  struct Record_return ret = Record('R', time0, 4, zipped.r0, zipped.r1);

  struct Bytes bytes = {.data = ret.r0, .len = (size_t)ret.r1};

  return bytes;
}

ID Rplain(const struct Bytes tlv) {
  struct Bytes value = LWWvalue(tlv);

  return UnzipID(value.data, value.len);
}

struct Bytes Rdelta(const struct Bytes old_tlv, ID new_val) {
  const struct Bytes new_tlv = Rtlv(new_val);

  return LWWdelta(old_tlv, new_tlv);
}

const struct Bytes Rmerge(const struct Bytes tlvs[], size_t tlvs_len) {
  return LWWmerge('R', tlvs, tlvs_len);
}
