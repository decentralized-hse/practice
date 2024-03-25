#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <tlv/io.hpp>
#include <variant>

namespace tlv {

namespace {

bool IsLong(char literal) {
  return 'A' <= literal && literal <= 'Z';
}

bool IsSmall(char literal) {
  return 'a' <= literal && literal <= 'z';
}

bool IsTiny(char literal) {
  return '0' <= literal && literal <= '9';
}

char UpperCaseTransfer(char literal) {
  if (IsLong(literal)) {
    return literal;
  }

  return literal - 32;
}

char LowerCaseTransfer(char literal) {
  if (IsSmall(literal)) {
    return literal;
  }

  return literal + 32;
}

}  // namespace

RecordWriter::RecordWriter(ll::Bytes bytes)
    : bytes_{std::move(bytes)} {
}

RecordWriter& RecordWriter::WriteRecord(char literal, const ll::Bytes& body,
                                        bool tiny) {
  if (tiny && body.size() <= 9) {
    bytes_.WriteLittleEndian('0' + body.size(), 1);
  } else {
    WriteHeader(literal, body.size());
  }

  bytes_.Append(body);

  return *this;
}

ll::Bytes RecordWriter::Extract() {
  return std::move(bytes_);
}

void RecordWriter::WriteHeader(char literal, size_t body_len) {
  assert(!IsTiny(literal));
  char long_literal = UpperCaseTransfer(literal);

  if (!IsLong(long_literal)) {
    assert(false && "TLV record type is A..Z");
  }

  if (body_len < 10 && IsSmall(literal)) {
    bytes_.WriteLittleEndian('0' + body_len, 1);
  } else if (body_len < 0xff) {
    bytes_.WriteLittleEndian(LowerCaseTransfer(literal), 1);
    bytes_.WriteLittleEndian(body_len, 1);
  } else {
    bytes_.WriteLittleEndian(long_literal, 1);
    bytes_.WriteLittleEndian(body_len, 4);
  }
}

RecordReader::RecordReader(ll::Bytes bytes)
    : bytes_(std::move(bytes)) {
}

Record RecordReader::ReadNext() {
  Header header = ReadHeader();

  if (bytes_.size() < header.body_size) {
    throw std::runtime_error("Invalid record");
  }

  ll::Bytes body = bytes_.Read(header.body_size);

  return {std::move(header), std::move(body)};
}

ll::Bytes RecordReader::Extract() {
  return std::move(bytes_);
}

Header RecordReader::ReadHeader() {
  if (bytes_.empty()) {
    throw std::runtime_error("Incomplete header");
  }

  char literal = bytes_.ReadLittleEndian(1);

  if (IsTiny(literal)) {
    return {'0', 1, (size_t)literal - '0'};
  } else if (IsSmall(literal)) {
    if (bytes_.size() < 2) {
      throw std::runtime_error("Incomplete header");
    }

    size_t body_len = bytes_.ReadLittleEndian(1);

    return {UpperCaseTransfer(literal), 2, body_len};
  } else if (IsLong(literal)) {
    if (bytes_.size() < 5) {
      throw std::runtime_error("Incomplete header");
    }

    size_t body_len = bytes_.ReadLittleEndian(4);

    return {literal, 5, body_len};
  } else {
    throw std::runtime_error("Bad format");
  }
}

}  // namespace tlv
