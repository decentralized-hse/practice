#include <gtest/gtest.h>

#include <isfr/types.hpp>
#include <ll/zipint.hpp>
#include <ll/bytes.hpp>

TEST(ISFR, TestTLV) {
  ll::Bytes body = {'t', 'e', 's', 't'};
  ll::Bytes tlv = isfr::Tlvt(body, isfr::Time{234, 123});

  auto [time, value] = isfr::Parse(std::move(tlv));

  ASSERT_EQ(time.revision, 234);
  ASSERT_EQ(time.source, 123);
  ASSERT_EQ(value, body);

  ll::Bytes doc = isfr::Tlvt(ll::Zip(int64_t(-11)), isfr::Time{4, 5});
  ASSERT_EQ(doc, (ll::Bytes{0x32, 0x08, 0x05, 0x15}));
}

TEST(ISFR, TestI) {
  ll::Bytes tlv1 = isfr::Iparse("123");
  ASSERT_EQ(tlv1, isfr::Itlv(123));

  ll::Bytes tlv2 = isfr::Iparse("345");
  ASSERT_EQ(345, isfr::Inative(tlv2));

  ll::Bytes delta = isfr::Idelta(tlv1, 345);
  ll::Bytes merged = isfr::Imerge({tlv1, delta});
  ASSERT_EQ(isfr::Istring(merged), "345");
}

TEST(ISFR, TestS) {
  std::string str1 = "a\nb\"\n";

  ll::Bytes tlv1 = isfr::Stlv(str1);
  std::string quoted = isfr::Sstring(tlv1);
  std::string unquoted = isfr::Snative(isfr::Sparse(quoted));
  ASSERT_EQ(str1, unquoted);
  ASSERT_EQ(str1, isfr::Snative(tlv1));

  std::string str2 = "g\n\"x\"\n";
  ll::Bytes delta = isfr::Sdelta(tlv1, str2);
  ll::Bytes merged = isfr::Smerge({tlv1, delta});
  ASSERT_EQ(str2, isfr::Snative(merged));
}

TEST(ISFR, TestF) {
  ll::Bytes tlv1 = isfr::Fparse("3.1415");
  ASSERT_EQ(3.1415, isfr::Fnative(tlv1));

  ll::Bytes tlv2 = isfr::Fparse("3.141592");
  ll::Bytes delta = isfr::Fdelta(tlv1, isfr::Fnative(tlv2));
  ll::Bytes merged = isfr::Fmerge({tlv1, delta});
  ASSERT_EQ("3.141592", isfr::Fstring(merged));
}

TEST(ISFR, TestIMerge) {
  ll::Bytes tlv1 = isfr::Itlv(123);
  ll::Bytes delta1 = isfr::Idelta(tlv1, 345);
  ll::Bytes merged1 = isfr::Imerge({tlv1, delta1});
  ASSERT_EQ(delta1, merged1);

  ll::Bytes tlv2 = isfr::Itlv(42, isfr::Time{100, 2});
  ll::Bytes merged12 = isfr::Imerge({tlv1, tlv2});
  ll::Bytes merged21 = isfr::Imerge({tlv2, tlv1});
  ASSERT_EQ(merged12, merged21);
}

TEST(ISFR, TestLWWTie) {
  ll::Bytes a = isfr::Tlvt(ll::Zip(int64_t(1)), isfr::Time{4, 8});
  ll::Bytes b = isfr::Tlvt(ll::Zip(int64_t(2)), isfr::Time{4, 7});
  ll::Bytes c = isfr::Tlvt(ll::Zip(int64_t(2)), isfr::Time{4, 5});

  ll::Bytes d = isfr::Imerge({a, b, c});
  auto [time, _] = isfr::Parse(d);
  ASSERT_EQ(isfr::Inative(d), 2);
  ASSERT_EQ(time.revision, 4);
  ASSERT_EQ(time.source, 7);
}
