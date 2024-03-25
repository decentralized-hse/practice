#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include <gtest/gtest.h>

#include <ll/zipint.hpp>

TEST(ZipInt, TestZigInt64) {
  std::vector<int64_t> tests = {
      0, -14, -10, 7, 20,
  };

  for (auto i : tests) {
    ll::Bytes bin = ll::Zip(i);
    ASSERT_EQ(i, ll::UnzipI64(bin));
  }
}

TEST(ZipInt, TestZigFloat) {
  std::map<double, int64_t> tests = {
      {0, 0},
      {1, 2},
      {1234, 3},
      {12.25, 3},
  };

  for (auto [d, len] : tests) {
    ll::Bytes bin = ll::Zip(d);
    ASSERT_EQ(bin.size(), len);
    ASSERT_EQ(d, ll::UnzipDouble(bin));
  }
}

TEST(ZipInt, TestZipU64Pair) {
  std::vector<uint64_t> nums = {
      0xca,
      0xbeff,
      0x12345678,
      0x7777777788888888,
  };

  for (size_t i = 0; i < nums.size(); ++i) {
    for (size_t j = 0; j < nums.size(); ++j) {
      ll::Bytes bin = ll::Zip(nums[i], nums[j]);
      auto [ri, rj] = ll::UnzipU64Pair(bin);
      ASSERT_EQ(ri, nums[i]);
      ASSERT_EQ(rj, nums[j]);
    }
  }
}

TEST(ZipInt, TestZipIU64Pair) {
  int64_t a = -7;
  uint64_t b = 15;

  ll::Bytes bin = ll::Zip(a, b);
  auto [i, u] = ll::UnzipIU64Pair(bin);

  ASSERT_EQ(a, i);
  ASSERT_EQ(b, u);
}
