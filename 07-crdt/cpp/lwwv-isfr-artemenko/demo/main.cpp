#include <cassert>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

#include <isfr/types.hpp>
#include <ll/zipint.hpp>
#include <ll/bytes.hpp>
#include <tlv/io.hpp>
#include <tlv/record.hpp>

ll::Bytes Init(char type) {
  if (type == 'I') {
    return isfr::Itlv({});
  } else if (type == 'S') {
    return isfr::Stlv({});
  } else if (type == 'R') {
    return isfr::Rtlv({});
  } else if (type == 'F') {
    return isfr::Ftlv({});
  }

  throw std::runtime_error("expected one of ISFR types");
}

ll::Bytes Merge(char type, ll::Bytes lhs, ll::Bytes rhs) {
  if (type == 'I') {
    return isfr::Imerge({std::move(lhs), std::move(rhs)});
  } else if (type == 'S') {
    return isfr::Smerge({std::move(lhs), std::move(rhs)});
  } else if (type == 'R') {
    return isfr::Rmerge({std::move(lhs), std::move(rhs)});
  } else if (type == 'F') {
    return isfr::Fmerge({std::move(lhs), std::move(rhs)});
  }

  throw std::runtime_error("expected one of ISFR types");
}

std::string String(char type, ll::Bytes tlv) {
  if (type == 'I') {
    return isfr::Istring(std::move(tlv));
  } else if (type == 'S') {
    return isfr::Sstring(std::move(tlv));
  } else if (type == 'R') {
    return isfr::Rstring(std::move(tlv));
  } else if (type == 'F') {
    return isfr::Fstring(std::move(tlv));
  }

  throw std::runtime_error("expected one of ISFR types");
}

int main() {
  char type = 'S';

  ll::Bytes merged = Init(type);
  std::string enveloped_tlv;

  while (std::getline(std::cin, enveloped_tlv)) {
    ll::Bytes bytes(enveloped_tlv.begin(), enveloped_tlv.end());
    tlv::RecordReader reader(std::move(bytes));

    while (reader.HasSome()) {
      tlv::Record tlv = reader.ReadNext();

      std::cout << "[" << tlv.header.literal
                << "] header size: " << tlv.header.header_size
                << ", body size: " << tlv.header.body_size << " | "
                << std::flush;

      auto [time, value] = isfr::Parse(tlv.body);

      std::cout << "time: {" << time.revision << ", " << time.source
                << "}, value: " << String(type, tlv.body) << std::endl;

      merged = Merge(type, std::move(merged), std::move(tlv.body));
    }

    assert(reader.IsEmpty());
  }

  std::cout << "merged: " << String(type, std::move(merged)) << std::endl;
}
