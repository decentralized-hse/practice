#pragma once

#include <string>
#include <vector>

#include <ll/bytes.hpp>
#include <cmn/id.hpp>

namespace isfr {

struct Time {
  int64_t revision = 0;
  uint64_t source = 0;
};

// Common
std::pair<Time, ll::Bytes> Parse(ll::Bytes value);
ll::Bytes Tlvt(ll::Bytes value, Time time);

// I
int64_t Inative(ll::Bytes tlv);
std::string Istring(ll::Bytes tlv);
ll::Bytes Itlv(int64_t value, Time time = {});
ll::Bytes Iparse(std::string text);
ll::Bytes Imerge(std::vector<ll::Bytes> tlvs);
ll::Bytes Idelta(ll::Bytes tlv, int64_t new_value);
bool Ivalid(ll::Bytes tlv);

// S
std::string Snative(ll::Bytes tlv);
std::string Sstring(ll::Bytes tlv);
ll::Bytes Sparse(std::string text);
ll::Bytes Stlv(std::string value, Time time = {});
ll::Bytes Smerge(std::vector<ll::Bytes> tlvs);
ll::Bytes Sdelta(ll::Bytes tlv, std::string new_value);
bool Svalid(ll::Bytes tlv);

// F
double Fnative(ll::Bytes tlv);
std::string Fstring(ll::Bytes tlv);
ll::Bytes Fparse(std::string text);
ll::Bytes Ftlv(double value, Time time = {});
ll::Bytes Fmerge(std::vector<ll::Bytes> tlvs);
ll::Bytes Fdelta(ll::Bytes tlv, double new_value);
bool Fvalid(ll::Bytes tlv);

// R
cmn::Id Rnative(ll::Bytes tlv);
std::string Rstring(ll::Bytes tlv);
ll::Bytes Rparse(std::string text);
ll::Bytes Rtlv(cmn::Id id, Time time = {});
ll::Bytes Rmerge(std::vector<ll::Bytes> tlvs);
ll::Bytes Rdelta(ll::Bytes tlv, cmn::Id new_value);
bool Rvalid(ll::Bytes tlv);

}  // namespace isfr
