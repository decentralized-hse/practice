#include <utils/bytes.hpp>

namespace tlv {

struct Header {
  char literal = 0;
  size_t header_size = 0;
  size_t body_size = 0;
};

struct Record {
  Header header;
  utils::Bytes body;
};

}  // namespace tlv
