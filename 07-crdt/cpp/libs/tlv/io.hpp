#pragma once

#include <optional>

#include <tlv/record.hpp>

namespace tlv {

class RecordWriter {
 public:
  RecordWriter() = default;
  explicit RecordWriter(ll::Bytes bytes);

  RecordWriter& WriteRecord(char literal, const ll::Bytes& body,
                            bool tiny = false);

  ll::Bytes Extract();

 private:
  void WriteHeader(char literal, size_t body_len);

 private:
  ll::Bytes bytes_;
};

class RecordReader {
 public:
  explicit RecordReader(ll::Bytes bytes);

  Record ReadNext();

  ll::Bytes Extract();

 private:
  Header ReadHeader();

 private:
  ll::Bytes bytes_;
};

}  // namespace tlv
