#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/zipint_tlv.h"
#include "lww.h"
#include "types.h"
#include "utils.h"

void TestInt64() {
  puts("================ TEST [int64] STARTED ================");

  const char* str1 = "123";
  const char* str2 = "345";

  struct Bytes tlv1 = Iparse(str1);
  PrintBytes("TLV1", tlv1);

  struct Bytes tlv_expected = Itlv(123);

  assert(Equals(tlv1, tlv_expected));

  struct Bytes tlv2 = Iparse(str2);
  PrintBytes("TLV2", tlv2);

  struct Bytes delta12 = Idelta(tlv1, 345);
  PrintBytes("DELTA12", delta12);

  const struct Bytes tlvs[2] = {tlv1, delta12};
  struct Bytes merged = Imerge(tlvs, 2);
  PrintBytes("MERGED", merged);

  char* merged_string = Istring(merged);
  PrintStringBytes("MERGED_S", merged_string);
  PrintStringBytes("STR2", str2);

  assert(strcmp(str2, merged_string) == 0);

  free(tlv1.data);
  free(tlv2.data);
  free(tlv_expected.data);
  free(delta12.data);
  free(merged_string);

  puts("================ TEST [int64] FINISHED ================\n");
}

void TestString() {
  puts("================ TEST [string] STARTED ================");

  const char* str1 = "fcuk\n\"zis\"\n";
  const char* str2 = "fcuk\n\"zat\"\n";

  struct Bytes tlv1 = Stlv(str1);
  PrintBytes("TLV1", tlv1);

  char* quoted = Sstring(tlv1);
  PrintStringBytes("QUOTED", quoted);

  struct Bytes parsed = Sparse(quoted);
  PrintBytes("PARSED", parsed);

  struct Bytes unquoted = LWWvalue(parsed);
  PrintBytes("UNQ", unquoted);
  PrintStringBytes("STR1", str1);
  assert(strncmp(str1, unquoted.data, unquoted.len) == 0);

  char* plain_tlv1 = Splain(tlv1);
  PrintStringBytes("PL_TLV1", plain_tlv1);
  assert(strcmp(str1, plain_tlv1) == 0);

  struct Bytes tlv2 = Stlv(str2);
  PrintBytes("TLV2", tlv2);

  struct Bytes delta12 = Sdelta(tlv1, str2);
  PrintBytes("DELTA12", delta12);

  const struct Bytes tlvs[2] = {tlv1, delta12};
  struct Bytes merged = Imerge(tlvs, 2);
  PrintBytes("MERGED", merged);

  char* plain_merged = Splain(merged);
  PrintStringBytes("PL_MERGED", plain_merged);
  assert(strcmp(str2, plain_merged) == 0);

  free(tlv1.data);
  free(tlv2.data);
  free(delta12.data);
  free(quoted);
  free(parsed.data);

  puts("================ TEST [string] FINISHED ================\n");
}

void TestFloat() {
  puts("================ TEST [float64] STARTED ================");

  const char* str1 = "3.1415";
  const char* str2 = "3.141592";

  struct Bytes tlv1 = Fparse(str1);
  PrintBytes("TLV1", tlv1);

  double id1 = 3.1415;
  double id2 = Fplain(tlv1);
  printf("ID1: %lf, ID2: %lf\n", id1, id2);
  assert(id1 == id2);

  struct Bytes tlv2 = Fparse(str2);
  PrintBytes("TLV2", tlv2);

  struct Bytes delta12 = Fdelta(tlv1, 3.141592);
  PrintBytes("DELTA12", delta12);

  const struct Bytes tlvs[2] = {tlv1, delta12};
  struct Bytes merged = Fmerge(tlvs, 2);
  PrintBytes("MERGED", merged);

  char* merged_string = Fstring(merged);
  PrintStringBytes("MER_S", merged_string);
  PrintStringBytes("STR2", str2);

  assert(strcmp(str2, merged_string) == 0);

  free(tlv1.data);
  free(tlv2.data);
  free(delta12.data);
  free(merged_string);

  puts("================ TEST [float64] FINISHED ================\n");
}

void TestID64() {
  puts("================ TEST [id64] STARTED ================");

  const char* str1 = "ae-32";
  const char* str2 = "ae-33";

  struct Bytes tlv1 = Rparse(str1);
  PrintBytes("TLV1", tlv1);

  ID id1 = MakeID(0xae, 0x32, 0x0);
  ID id2 = Rplain(tlv1);
  printf("ID1: %lx, ID2: %lx\n", id1, id2);
  assert(id1 == id2);

  struct Bytes tlv2 = Rparse(str2);
  PrintBytes("TLV2", tlv2);

  struct Bytes delta12 = Rdelta(tlv1, MakeID(0xae, 0x33, 0x0));
  PrintBytes("DELTA12", delta12);

  const struct Bytes tlvs[2] = {tlv1, delta12};
  struct Bytes merged = Rmerge(tlvs, 2);
  PrintBytes("MERGED", merged);

  char* merged_string = Rstring(merged);
  PrintStringBytes("MER_S", merged_string);
  PrintStringBytes("STR2", str2);

  assert(strcmp(str2, merged_string) == 0);

  free(tlv1.data);
  free(tlv2.data);
  free(delta12.data);
  free(merged_string);

  puts("================ TEST [id64] FINISHED ================\n");
}

int main(void) {
  TestInt64();
  TestString();
  TestFloat();
  TestID64();
}
