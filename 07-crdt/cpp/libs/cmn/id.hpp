#pragma once

#include <cstdint>
#include <string>

#include <ll/bytes.hpp>

namespace cmn {

struct Id {
  uint64_t offset : 12 = 0;
  uint64_t sequence : 32 = 0;
  uint64_t source : 20 = 0;

  auto operator<=>(const Id& rhs) const = default;
};

static_assert(sizeof(Id) == sizeof(uint64_t), "Invalid ID structure");

ll::Bytes Zip(Id id);
Id UnzipID(ll::Bytes bytes);

std::string ToString(Id id);
Id FromString(std::string text);

}  // namespace cmn
