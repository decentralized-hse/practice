#include <cstddef>
#include <ios>
#include <ll/zipint.hpp>

#include <cmn/id.hpp>
#include <sstream>
#include <stdexcept>

namespace cmn {

namespace {

uint64_t GetProgress(Id id) {
  uint64_t offset = id.offset;
  uint64_t sequence = id.sequence;

  return (offset << 32) | sequence;
}

}  // namespace

ll::Bytes Zip(Id id) {
  return ll::Zip(GetProgress(id), id.source);
}

Id UnzipID(ll::Bytes bytes) {
  auto [big, lil] = ll::UnzipU64Pair(bytes);

  Id id;
  id.offset = big >> 32;
  id.sequence = big & 0xffffffff;
  id.source = lil;

  return id;
}

std::string ToString(Id id) {
  std::stringstream ss;

  if (id.source == 0 && id.sequence == 0) {
    ss << std::hex << id.offset;
  } else if (id.offset == 0) {
    ss << std::hex << id.source << "-" << id.sequence;
  } else {
    ss << std::hex << id.source << "-" << id.sequence << "-" << id.offset;
  }

  return ss.str();
}

Id FromString(std::string text) {
  std::stringstream ss(text);

  size_t i = 0;
  uint64_t parts[3] = {};

  char c;
  for (; i < 3; ++i) {
    bool new_part = bool(ss >> std::hex >> parts[i]);

    if (!new_part) {
      throw std::runtime_error("Incorrect Id format");
    }

    if (ss.eof()) {
      break;
    }

    ss >> c;

    if (c != '-') {
      throw std::runtime_error("Incorrect Id format");
    }
  }

  if (!ss.eof()) {
    throw std::runtime_error("Incorrect Id format");
  }

  Id id;

  if (i == 0) {
    id.offset = parts[0];
  } else if (i == 1) {
    id.source = parts[0];
    id.sequence = parts[1];
  } else {
    id.source = parts[0];
    id.sequence = parts[1];
    id.offset = parts[2];
  }

  return id;
}

}  // namespace cmn
