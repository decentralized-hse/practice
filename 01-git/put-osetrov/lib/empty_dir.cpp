#include "empty_dir.hpp"
#include "sha256.hpp"

#include <fstream>

void CreateEmptyIfNotExists() {
  static const auto empty_file_hash = ComputeHash("");

  std::fstream empty_file;
  empty_file.open(empty_file_hash, std::ios::out | std::ios::app);
  empty_file.close();
}
