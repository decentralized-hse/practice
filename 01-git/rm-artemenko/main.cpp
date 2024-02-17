#include <cassert>
#include <cstddef>
#include <vector>

#include <argparse/argparse.hpp>

#include <tree.hpp>

void CheckArgs(const std::string &path, const std::string &root_hash) {
  static const std::regex path_matcher("^[^\t :]+$");
  static const std::regex hash_matcher("^[a-fA-F0-9]{64}$");

  if (!std::regex_match(path, path_matcher)) {
    throw std::logic_error("incorrect format of path");
  }

  if (!std::regex_match(root_hash, hash_matcher)) {
    throw std::logic_error("incorrect format of root hash");
  }
}

std::vector<std::string> SplitPath(std::string path) {
  std::vector<std::string> parts;

  while (!path.empty()) {
    size_t ind = path.find("/");

    if (ind == std::string::npos) {
      parts.push_back(std::move(path));
      path = "";
    } else {
      std::string subdir = path.substr(0, ind);
      parts.push_back(std::move(subdir));
      path = path.substr(ind + 1);
    }
  }

  return parts;
}

std::string RunRemove(std::vector<std::string> path_parts, std::string root_hash) {
  std::vector<Tree> trees = {Tree::fromHash(root_hash)};

  for (size_t i = 0; i + 1 < path_parts.size(); ++i) {
    auto entry = trees.back().FindEntry(path_parts[i]);

    if (entry.type != EntryType::Tree) {
      throw std::runtime_error(fmt::format("'{}' is not a folder", path_parts[i]));
    }

    trees.push_back(Tree::fromHash(entry.hash));
  }

  assert(trees.size() == path_parts.size());

  std::reverse(trees.begin(), trees.end());
  std::reverse(path_parts.begin(), path_parts.end());

  trees[0].RemoveEntry(path_parts[0]);
  trees[0].WriteToFile();

  for (size_t i = 1; i < trees.size(); ++i) {
    trees[i].ReplaceHash(path_parts[i], trees[i - 1].CalculateHash());
    trees[i].WriteToFile();
  }

  return trees.back().CalculateHash();
}

int main(int argc, char *argv[]) try {
  argparse::ArgumentParser parser("rm");

  parser.add_argument("path").help("path to remove").required();
  parser.add_argument("root_hash").help("hash of root tree").required();

  parser.parse_args(argc, argv);

  std::string path = parser.get("path");
  std::string root_hash = parser.get("root_hash");
  CheckArgs(path, root_hash);

  auto path_parts = SplitPath(path);

  std::cout << RunRemove(std::move(path_parts), root_hash) << std::endl;

  return 0;

} catch (const std::exception &e) {
  std::cerr << e.what() << std::endl;
  return 1;
}
