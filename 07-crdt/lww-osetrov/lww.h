#pragma once

#include <stdint.h>

#include "types.h"
#include "utils.h"

//////////////////// SHARED ////////////////////

struct Bytes LWWvalue(const struct Bytes tlv);

struct Bytes LWWdelta(const struct Bytes old_tlv, const struct Bytes new_tlv);

const struct Bytes LWWmerge(char lit, const struct Bytes tlvs[],
                            size_t tlvs_len);

//////////////////// INT64 ////////////////////

bool Ivalid(const struct Bytes tlv);

char* Istring(const struct Bytes tlv);

struct Bytes Iparse(const char* txt);

struct Bytes Itlv(int64_t i);

int64_t Iplain(const struct Bytes tlv);

struct Bytes Idelta(const struct Bytes old_tlv, int64_t new_val);

const struct Bytes Imerge(const struct Bytes tlvs[], size_t tlvs_len);

//////////////////// STRING ////////////////////

bool Svalid(const struct Bytes tlv);

char* Sstring(const struct Bytes tlv);

struct Bytes Sparse(const char* txt);

struct Bytes Stlv(const char* txt);

char* Splain(const struct Bytes tlv);

struct Bytes Sdelta(const struct Bytes old_tlv, const char* new_val);

const struct Bytes Smerge(const struct Bytes tlvs[], size_t tlvs_len);

//////////////////// FLOAT64 ////////////////////

bool Fvalid(const struct Bytes tlv);

char* Fstring(const struct Bytes tlv);

struct Bytes Fparse(const char* txt);

struct Bytes Ftlv(double f);

double Fplain(const struct Bytes tlv);

struct Bytes Fdelta(const struct Bytes old_tlv, double new_val);

const struct Bytes Fmerge(const struct Bytes tlvs[], size_t tlvs_len);

//////////////////// ID64 ////////////////////

bool Rvalid(const struct Bytes tlv);

char* Rstring(const struct Bytes tlv);

struct Bytes Rparse(const char* txt);

struct Bytes Rtlv(ID id);

ID Rplain(const struct Bytes tlv);

struct Bytes Rdelta(const struct Bytes old_tlv, ID new_val);

const struct Bytes Rmerge(const struct Bytes tlvs[], size_t tlvs_len);
