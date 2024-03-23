#include <gtest/gtest.h>

#include <tlv/record.hpp>
#include <utils/zipint.hpp>

TEST(Simple, A) {
  tlv::PrintHello();
  utils::PrintBye();

  ASSERT_TRUE(true);
}
