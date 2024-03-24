#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <utils/zipint.hpp>
#include <vector>

TEST(ZipInt, TestZigInt64) {
  std::vector<int64_t> tests = {
      0, -14, -10, 7, 20,
  };

  for (auto i : tests) {
    utils::Bytes bin = utils::Zip(i);
    ASSERT_EQ(i, utils::UnzipI64(bin));
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
    utils::Bytes bin = utils::Zip(d);
    ASSERT_EQ(bin.size(), len);
    ASSERT_EQ(d, utils::UnzipDouble(bin));
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
      utils::Bytes bin = utils::Zip(nums[i], nums[j]);
      auto [ri, rj] = utils::UnzipU64Pair(bin);
      ASSERT_EQ(ri, nums[i]);
      ASSERT_EQ(rj, nums[j]);
    }
  }
}

TEST(ZipInt, TestZipIU64Pair) {
  int64_t a = -7;
  uint64_t b = 15;

  utils::Bytes bin = utils::Zip(a, b);
  auto [i, u] = utils::UnzipIU64Pair(bin);

  ASSERT_EQ(a, i);
  ASSERT_EQ(b, u);
}
