#include "sha256.hpp"

#include "picosha2.h"

#include <algorithm>

std::string ComputeHash(const std::string& data) {
  return picosha2::hash256_hex_string(data);
}

bool IsValidSHA256(const std::string& data) {
  if (data.size() != 2 * picosha2::k_digest_size) {
    return false;
  }

  return std::ranges::all_of(data, [](char c) {
    return std::isdigit(c) || ('a' <= c && c <= 'f');
  });
}
