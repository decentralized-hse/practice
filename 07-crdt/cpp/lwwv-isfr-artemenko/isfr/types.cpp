#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <ll/zipint.hpp>
#include <ll/bytes.hpp>
#include <tlv/io.hpp>

#include <isfr/types.hpp>

namespace isfr {

namespace {

ll::Bytes ToBytes(Time time) {
  return ll::Zip(time.revision, time.source);
}

ll::Bytes ISFRmerge(std::vector<ll::Bytes> tlvs) {
  Time max_time;
  ll::Bytes max_value;

  for (auto&& tlv : tlvs) {
    auto [time, value] = Parse(tlv);
    time.revision = std::abs(time.revision);

    auto cur_tie = std::tie(time.revision, value, time.source);
    auto max_tie = std::tie(max_time.revision, max_value, max_time.source);

    if (cur_tie > max_tie) {
      max_time = std::move(time);
      max_value = std::move(value);
    }
  }

  return Tlvt(std::move(max_value), std::move(max_time));
}

template <class T>
T ReadFromText(const std::string& text) {
  std::stringstream ss(text);
  ss.exceptions(std::ios::failbit);

  T value;
  ss >> value;

  return value;
}

}  // namespace

/////////////////////// Common

std::pair<Time, ll::Bytes> Parse(ll::Bytes value) {
  tlv::RecordReader reader(std::move(value));
  tlv::Record record = reader.ReadNext();

  if (record.header.literal != 'T' && record.header.literal != '0') {
    throw std::runtime_error("invalid ISFR");
  }

  auto [rev, src] = ll::UnzipIU64Pair(record.body);

  return {Time{rev, src}, reader.Extract()};
}

ll::Bytes Tlvt(ll::Bytes value, Time time) {
  ll::Bytes time_bytes = ToBytes(time);

  tlv::RecordWriter writer;
  writer.WriteRecord('T', time_bytes, true);
  ll::Bytes tlv = writer.Extract();

  tlv.Append(std::move(value));

  return tlv;
}

/////////////////////// I

int64_t Inative(ll::Bytes tlv) {
  auto [_, value] = Parse(std::move(tlv));
  return ll::UnzipI64(value);
}

std::string Istring(ll::Bytes tlv) {
  return std::to_string(Inative(std::move(tlv)));
}

ll::Bytes Itlv(int64_t value, Time time) {
  return Tlvt(ll::Zip(value), time);
}

ll::Bytes Iparse(std::string text) {
  return Itlv(ReadFromText<int64_t>(text));
}

ll::Bytes Imerge(std::vector<ll::Bytes> tlvs) {
  return ISFRmerge(std::move(tlvs));
}

ll::Bytes Idelta(ll::Bytes tlv, int64_t new_value) {
  auto [time, value] = Parse(std::move(tlv));
  time.revision = std::abs(time.revision);

  return Tlvt(ll::Zip(new_value), {time.revision + 1, 0});
}

bool Ivalid(ll::Bytes tlv) {
  auto [time, value] = Parse(std::move(tlv));
  return 0 < value.size() && value.size() < 8;
}

/////////////////////// S

std::string Snative(ll::Bytes tlv) {
  auto [_, value] = Parse(std::move(tlv));
  return std::string(value.begin(), value.end());
}

std::string Sstring(ll::Bytes tlv) {
  auto [_, value] = Parse(std::move(tlv));

  std::stringstream ss;
  ss << '"';

  for (char byte : value) {
    switch (byte) {
      case '\\':
      case '"':
        ss << "\\" << byte;
        break;
      case '\n':
        ss << "\\n";
        break;
      case '\r':
        ss << "\\r";
        break;
      case '\t':
        ss << "\\t";
        break;
      default:
        if (byte < 0x20) {
          ss << "\\u00" << std::hex << static_cast<int>(byte);
        } else {
          ss << byte;
        }
    }
  }

  ss << '"';

  return ss.str();
}

ll::Bytes Sparse(std::string text) {
  if (text.size() < 2 || text.front() != '"' || text.back() != '"') {
    throw std::runtime_error("Incorrect text");
  }

  ll::Bytes value;

  for (size_t i = 1; i + 1 < text.size(); ++i) {
    if (text[i] != '\\') {
      value.push_back(text[i]);
      continue;
    }

    if (i + 2 >= text.size()) {
      throw std::runtime_error("Invalid escape sequence");
    }

    switch (text[i + 1]) {
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      case '\\':
        value.push_back('\\');
        break;
      case '"':
        value.push_back('"');
        break;
      default:
        throw std::runtime_error("Unrecognized escape sequence: \\" +
                                 std::string(1, text[i + 1]));
    }

    ++i;
  }

  return Tlvt(std::move(value), {});
}

ll::Bytes Stlv(std::string value, Time time) {
  return Tlvt(ll::Bytes(value.begin(), value.end()), std::move(time));
}

ll::Bytes Smerge(std::vector<ll::Bytes> tlvs) {
  return ISFRmerge(std::move(tlvs));
}

ll::Bytes Sdelta(ll::Bytes tlv, std::string new_value) {
  auto [time, value] = Parse(std::move(tlv));
  time.revision = std::abs(time.revision);

  return Tlvt(ll::Bytes(new_value.begin(), new_value.end()),
              {time.revision + 1, 0});
}

bool Svalid(ll::Bytes tlv) {
  auto [time, value] = Parse(std::move(tlv));
  return !value.empty();
}

/////////////////////// F

double Fnative(ll::Bytes tlv) {
  auto [_, value] = Parse(std::move(tlv));
  return ll::UnzipDouble(std::move(value));
}

std::string Fstring(ll::Bytes tlv) {
  return std::to_string(Fnative(std::move(tlv)));
}

ll::Bytes Fparse(std::string text) {
  return Ftlv(ReadFromText<double>(text));
}

ll::Bytes Ftlv(double value, Time time) {
  return Tlvt(ll::Zip(value), std::move(time));
}

ll::Bytes Fmerge(std::vector<ll::Bytes> tlvs) {
  return ISFRmerge(std::move(tlvs));
}

ll::Bytes Fdelta(ll::Bytes tlv, double new_value) {
  auto [time, value] = Parse(std::move(tlv));
  time.revision = std::abs(time.revision);

  return Tlvt(ll::Zip(new_value), {time.revision + 1, 0});
}

bool Fvalid(ll::Bytes tlv) {
  auto [time, value] = Parse(std::move(tlv));
  return 0 <= value.size() && value.size() <= 8;
}

}  // namespace isfr
