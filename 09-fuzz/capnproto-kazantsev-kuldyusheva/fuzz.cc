#include <stdint.h>
#include <stddef.h>
#include "validate.h"
#include "capnproto-kazantsev-kuldyusheva.c++"
#include <iostream>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  std::string name = "fuzz";

  std::ofstream out(name + ".bin", std::ios::binary | std::ios::trunc);
  out.write((char*)Data, Size);
  out.close();

  if(!validateData(name)) {
    return 0;
  }

  FromStudentToCapnproto(name);
  FromCapnprotoToStudent(name);

  std::ifstream in(name + ".bin", std::ios::binary);
  std::vector<uint8_t> data(std::istreambuf_iterator<char>(in), {});
  auto n = memcmp(data.data(), Data, Size);
  if (n != 0) {
    throw std::runtime_error("Invalid fuzzing");
  }
  return 0;
}
