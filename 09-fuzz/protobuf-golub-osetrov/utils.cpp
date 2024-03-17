#include "utils.hpp"

#include <algorithm>

bool ends_with(const std::string &str, const std::string &end) {
  if (end.size() > str.size()) {
    return false;
  }
  return std::equal(end.rbegin(), end.rend(), str.rbegin());
}
