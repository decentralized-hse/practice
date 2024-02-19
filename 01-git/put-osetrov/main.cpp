#include <iostream>

#include "lib/empty_dir.hpp"
#include "lib/git_put.hpp"
#include "lib/read_content.hpp"
#include "lib/sha256.hpp"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <path> <root-hash>" << std::endl;
    std::exit(1);
  }

  CreateEmptyIfNotExists();

  const std::string file_path{argv[1]};
  const std::string root_hash{argv[2]};

  if (!IsValidSHA256(root_hash)) {
    std::cerr << "Given hash is not valid SHA-256 hash hex string" << std::endl;
    std::exit(1);
  }

  const std::string file_content = ReadContent();

  if (const auto new_root_hash = GitPut(root_hash, file_path, file_content)) {
    std::cout << *new_root_hash << std::endl;
    std::exit(0);
  } else {
    std::cerr << "Git put failed" << std::endl;
    std::exit(1);
  }
}
