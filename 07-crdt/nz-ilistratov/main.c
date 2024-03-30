#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/zipint_tlv.h"
#include "nz.h"
#include "types.h"
#include "utils.h"

const char* n_str1 = "1 ff 0";
const char* n_str2 = "2 ff 0 1 1";
const char* n_str_delta = "1 1 1";

struct URecord n_rec_1_arr[] = {
  { 0xffull, 0ull },
};

struct NRecord n_rec1 = {
  .size = 1,
  .data = n_rec_1_arr
};

struct URecord n_rec_2_arr[] = {
  { 0xffull, 0ull },
  { 1ull, 1ull },
};

struct NRecord n_rec2 = {
  .size = 2,
  .data = n_rec_2_arr
};

struct URecord n_rec_delta_arr[] = {
  { 1ull, 1ull },
};

struct NRecord n_rec_delta = {
  .size = 1,
  .data = n_rec_delta_arr
};

void TestN() {
  puts("================ TEST [N] STARTED ================");

  struct Bytes tlv1 = Nparse(n_str1);
  PrintBytes("TLV1", tlv1);
  struct Bytes tlv1_expected = Ntlv(n_rec1);
  char* tlv1_str = Nstring(tlv1);
  assert(strcmp(n_str1, tlv1_str) == 0);
  assert(Equals(tlv1, tlv1_expected));

  struct Bytes tlv2 = Nparse(n_str2);
  PrintBytes("TLV2", tlv2);
  struct Bytes tlv2_expected = Ntlv(n_rec2);
  char* tlv2_str = Nstring(tlv2);
  assert(strcmp(n_str2, tlv2_str) == 0);
  assert(Equals(tlv2, tlv2_expected));

  struct Bytes delta12 = Ndelta(tlv1, n_rec2);
  PrintBytes("DELTA12", delta12);
  char* delta_str = Nstring(delta12);
  assert(strcmp(n_str_delta, delta_str) == 0);

  const struct Bytes tlvs[2] = {tlv1, delta12};
  struct Bytes merged = Nmerge(tlvs, 2);
  PrintBytes("MERGED", merged);

  char* merged_string = Nstring(merged);
  PrintStringBytes("MERGED_S", merged_string);
  PrintStringBytes("STR2", n_str2);

  assert(strcmp(n_str2, merged_string) == 0);

  free(tlv1.data);
  free(tlv1_expected.data);
  free(tlv1_str);
  free(tlv2.data);
  free(tlv2_expected.data);
  free(tlv2_str);
  free(delta12.data);
  free(delta_str);
  free(merged.data);
  free(merged_string);

  puts("================ TEST [int64] FINISHED ================\n");
}

int main(void) {
  TestN();
}
