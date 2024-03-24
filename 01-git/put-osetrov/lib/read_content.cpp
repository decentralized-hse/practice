#include "read_content.hpp"

#include <unistd.h>

#include <array>

std::string ReadContent() {
  std::string result;

  std::array<char, 4096> buf;
  for (;;) {
    const ssize_t bytes = read(STDIN_FILENO, buf.data(), buf.size());
    if (bytes <= 0) {
      break;
    }
    result.append(buf.begin(), buf.begin() + bytes);
  }

  return result;
}
