#include "git_put.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "sha256.hpp"

namespace {

std::pair<std::string, std::vector<std::string>> SplitPath(
    const std::string& file_path) {
  std::filesystem::path path{file_path};

  std::string filename = path.filename();
  path = path.parent_path();

  std::vector<std::string> dirs;
  for (const auto& dir_name : path) {
    dirs.emplace_back(std::format("{}/", dir_name.string()));
  }

  return {std::move(filename), std::move(dirs)};
}

std::map<std::string, std::string> ParseDir(const std::string& dir_hash) {
  std::map<std::string, std::string> hash_by_name;

  {
    std::ifstream dir{dir_hash};

    for (std::string line; std::getline(dir, line);) {
      std::string name;
      std::string hash;

      std::istringstream ss{line};
      ss >> name >> hash;

      hash_by_name.emplace_hint(hash_by_name.end(), std::move(name),
                                std::move(hash));
    }

    dir.close();
  }

  return hash_by_name;
}

std::string CreateBlob(const std::string& file_content) {
  std::string content_hash = ComputeHash(file_content);

  std::ofstream file{content_hash};
  file.write(file_content.data(), file_content.size());
  file.close();

  return content_hash;
}

std::string CreateDir(const std::map<std::string, std::string>& hash_by_name) {
  std::stringstream ss;
  for (const auto& [name, hash] : hash_by_name) {
    ss << std::format("{}\t{}\n", name, hash);
  }
  const std::string result_content{ss.str()};

  return CreateBlob(result_content);
}

}  // namespace

std::optional<std::string> GitPut(const std::string& root_hash,
                                  const std::string& file_path,
                                  const std::string& file_content) {
  if (!std::filesystem::exists(root_hash)) {
    std::cerr << "Root directory does not exists" << std::endl;
    return std::nullopt;
  }

  const auto [filename, dirs] = SplitPath(file_path);

  std::vector<std::string> dir_hashes{root_hash};
  dir_hashes.reserve(dirs.size() + 1);
  std::unordered_map<std::string, std::string> dir_name_by_hash;
  dir_name_by_hash.reserve(dirs.size() + 1);
  dir_name_by_hash.emplace(root_hash, "dummy");
  {
    std::string cur_hash{root_hash};
    for (size_t cur_dir = 0; cur_dir < dirs.size(); ++cur_dir) {
      if (!std::filesystem::exists(cur_hash)) {
        std::cerr << std::format("No file with hash {}\n", cur_hash);
        return std::nullopt;
      }

      const auto hash_by_name = ParseDir(cur_hash);

      auto next_hash_it = hash_by_name.find(dirs.at(cur_dir));
      if (next_hash_it == hash_by_name.end()) {
        std::cerr << std::format("Not found reference to {} in {}\n",
                                 dirs.at(cur_dir), cur_hash);
        return std::nullopt;
      }

      dir_hashes.push_back(std::move(next_hash_it->second));
      cur_hash = dir_hashes.back();
      dir_name_by_hash.emplace(cur_hash, dirs.at(cur_dir));
    }
  }
  std::ranges::reverse(dir_hashes);

  std::string new_filename = std::format("{}:", filename);
  std::string new_file_hash = CreateBlob(file_content);
  for (const auto& dir_hash : dir_hashes) {
    auto dir = ParseDir(dir_hash);
    dir.emplace(new_filename, new_file_hash);

    new_filename = dir_name_by_hash.at(dir_hash);
    new_file_hash = CreateDir(dir);
  }

  return new_file_hash;
}
