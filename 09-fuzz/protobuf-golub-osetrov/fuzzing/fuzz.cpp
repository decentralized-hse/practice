#include <fcntl.h>
#include <unistd.h>
#include <utf8_validity.h>

#include <cstdio>
#include <fstream>
#include <sstream>

#include "../protobuf-golub-osetrov.hpp"
#include "../utils.hpp"

namespace {

void SaveDataToFile(const std::string& path, const uint8_t* data, size_t size) {
  std::ofstream ofs{
      path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
  ofs.write(reinterpret_cast<const char*>(data), size);
  ofs.close();
}

std::string ReadFileToString(const std::string& path) {
  std::ifstream ifs{path, std::ios_base::in | std::ios_base::binary};
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

bool CheckStr(const char* str, int len, const char* name) {
  int i = 0;
  while (i < len && str[i] != 0) i++;
  while (i < len && str[i] == 0) i++;
  return i == len && str[len - 1] == '\0' &&
         utf8_range::IsStructurallyValid(str);
  /// @note libprotobuf uses utf8_range same way for parsing:
  /// https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/wire_format_lite.cc#L608
}

bool CheckInput(const std::string& path) {
  Student student;
  constexpr int RECSIZE = sizeof(struct Student);
  int fd = open(path.data(), O_RDONLY);
  if (fd == -1) {
    return false;
  }
  ssize_t rd = 0;
  bool one = false;
  while (RECSIZE == (rd = read(fd, &student, RECSIZE))) {
    if (!CheckStr(student.name, 32, "name")) {
      return false;
    }
    if (!CheckStr(student.login, 16, "login")) {
      return false;
    }
    if (!CheckStr(student.group, 8, "group")) {
      return false;
    }
    for (int p = 0; p < 8; p++) {
      if (student.practice[p] != 0 && student.practice[p] != 1) {
        return false;
      }
    }
    if (!CheckStr(student.project.repo, 59, "repo")) {
      return false;
    }
    if (student.project.mark < 0 || student.project.mark > 10) {
      return false;
    }
    if (student.mark < 0 || student.mark > 10) {
      return false;
    }
    one = true;
  }
  close(fd);

  if (rd > 0 && rd % RECSIZE != 0) {
    return false;
  }

  return one;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string tmp_file = "/tmp/fuzz.XXXXXX";
  int temp_file = mkstemp(tmp_file.data());
  const std::string temp_proto =
      tmp_file.substr(0, tmp_file.size() - 4) + ".protobuf";
  const std::string temp_bin =
      temp_proto.substr(0, temp_proto.size() - 9) + ".bin";

  const auto clear = [temp_file, &tmp_file, &temp_proto, &temp_bin] {
    close(temp_file);
    std::remove(tmp_file.data());
    std::remove(temp_proto.data());
    std::remove(temp_bin.data());
  };
  const auto show_files = [&tmp_file, &temp_proto, &temp_bin] {
    std::cerr << "Initial file: " << tmp_file << ", Proto: " << temp_proto
              << ", Bin: " << temp_bin << std::endl;
  };

  SaveDataToFile(tmp_file, data, size);

  const bool correct_input = CheckInput(tmp_file);
  const auto err = SerializeToProtobuf(tmp_file);

  if (!correct_input) {
    if (err != Error::MALFORMED_INPUT) {
      std::cerr << "Expected malformed input, got: " << ErrorToString(err)
                << ", data size: " << size << std::endl;
      show_files();
      std::exit(1);
    }
    clear();
    return 0;
  }
  if (err != Error::NO_ERROR) {
    std::cerr << "Expected no error, got: " << ErrorToString(err) << std::endl;
    show_files();
    std::exit(2);
  }

  if (auto err = SerializeToBin(temp_proto); err != Error::NO_ERROR) {
    std::cerr << "Error: " << ErrorToString(err) << std::endl;
    show_files();
    std::exit(3);
  }

  const std::string round_trip_result = ReadFileToString(temp_bin);

  if (round_trip_result.size() != size) {
    std::cerr << "size: " << round_trip_result.size()
              << ", expected size: " << size << std::endl;
    show_files();
    std::exit(4);
  }

  if (std::memcmp(data, round_trip_result.data(), size) != 0) {
    std::cerr << "Not binary equal data. Data size: " << size << std::endl;
    for (size_t i = 0; i < size; ++i) {
      if (data[i] != round_trip_result.at(i)) {
        std::cerr << "Differ in byte " << i
                  << ". Expected: " << static_cast<int>(data[i])
                  << ", Got: " << static_cast<int>(round_trip_result.at(i))
                  << std::endl;
      }
    }
    show_files();
    std::exit(5);
  }

  clear();
  return 0;
}
