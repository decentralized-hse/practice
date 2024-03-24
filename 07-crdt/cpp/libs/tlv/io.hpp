#include <optional>

#include <tlv/record.hpp>

namespace tlv {

class RecordWriter {
 public:
  RecordWriter() = default;
  explicit RecordWriter(utils::Bytes bytes);

  RecordWriter& WriteRecord(char literal, const utils::Bytes& body,
                            bool tiny = false);

  utils::Bytes Extract();

 private:
  void WriteHeader(char literal, size_t body_len);

 private:
  utils::Bytes bytes_;
};

class RecordReader {
 public:
  explicit RecordReader(utils::Bytes bytes);

  Record ReadNext();

 private:
  Header ReadHeader();

 private:
  utils::Bytes bytes_;
};

}  // namespace tlv
