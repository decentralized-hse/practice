#include <iostream>

#include "protobuf-golub-osetrov.hpp"
#include "utils.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Specify the path to the input file to convert as arg"
              << std::endl;
    return 1;
  }
  const std::string path = argv[1];
  if (ends_with(path, ".bin")) {
    if (auto err = SerializeToProtobuf(path); err != Error::NO_ERROR) {
      std::cerr << ErrorToString(err) << std::endl;
      std::exit(-1);
    };
  } else if (ends_with(path, ".protobuf")) {
    if (auto err = SerializeToBin(path); err != Error::NO_ERROR) {
      std::cerr << ErrorToString(err) << std::endl;
      std::exit(-1);
    };
  } else {
    std::cerr << "Invalid file name" << std::endl;
    return 1;
  }
}
