#pragma once

#include <cstdint>

#include "protobuf-golub-osetrov.pb.h"

struct Student {
  // имя может быть и короче 32 байт, тогда в хвосте 000
  // имя - валидный UTF-8
  char name[32] = {0};

  // ASCII [\w]+
  char login[16] = {0};
  char group[8] = {0};

  // 0/1, фактически bool
  uint8_t practice[8] = {0};

  struct {
    // URL
    char repo[59] = {0};
    uint8_t mark = 0;
  } project;

  // 32 bit IEEE 754 float
  float mark = 0;
};

enum class Error {
  NO_ERROR = 1,
  FAILED_TO_SERIALIZE_TO_PROTO = 2,
  FAILED_TO_PARSE_PROTO = 3,
};
std::string ErrorToString(Error error);

void buildProtoStudent(format04::Student* res, const Student& student);
Student parseProtoStudent(const format04::Student& student);

[[nodiscard]] Error SerializeToProtobuf(const std::string& path);
[[nodiscard]] Error SerializeToBin(const std::string& path);
