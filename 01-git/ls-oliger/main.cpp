#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string DirectorySearch(const std::string &path, const std::string &root)
{
  // std::cout << "DEBUG | DirectorySearch(" << path << "; " << root << ")\n";
  if (path == "." || path == "./" || path == "")
  {
    return root;
  }
  std::vector<std::string> tokens;
  std::istringstream iss(path);
  std::string token;
  while (std::getline(iss, token, '/'))
  {
    tokens.push_back(token);
  }
  if (tokens.size() < 2)
  {
    std::cerr << "No such directory found. Details:\n " << "Path: " << path << "\nRoot:" << root << "\n";
    throw std::runtime_error("Incorrect input (1)");
  }
  std::string pathPrefix = tokens[0] + "/";
  std::string pathSuffix = tokens[1];
  std::ifstream file(root);
  if (!file)
  {
    std::cerr << "Could not find or open file in given directory. Details:\n " << "Path: " << path << "\nRoot:" << root << "\n";
    throw std::runtime_error("Incorrect input (2)");
  }
  std::string line;
  while (std::getline(file, line))
  {
    std::vector<std::string> inputLines;
    std::istringstream iss(line);
    while (std::getline(iss, token, '\t'))
    {
      inputLines.push_back(token);
    }
    if (pathPrefix == inputLines[0])
    {
      return DirectorySearch(pathSuffix, inputLines[1]);
    }
  }
  std::cerr << "No such prefix found. Details:\n " << "Prefix: " << pathPrefix << "\nSuffix: " << pathSuffix << "\n";
  throw std::runtime_error("Incorrect input (3)");
}

void DirectoryOutput(const std::string &hash, const std::string &prefix,
                     bool isRoot)
{
  // std::cout << "DEBUG | DirectoryOutput(" << hash << "; " << prefix
  //           << "; " << isRoot << ")\n";
  std::ifstream file(hash);
  if (!file)
  {
    std::cerr << "Could not open file. Details:\n " << "Hash: " << hash << "\nPrefix:" << prefix << "\n";
    throw std::runtime_error("Incorrect input (2)");
  }
  std::string line;
  while (std::getline(file, line))
  {
    std::vector<std::string> inputLines;
    std::istringstream iss(line);
    std::string token;
    while (std::getline(iss, token, '\t'))
    {
      inputLines.push_back(token);
    }
    std::string objectName = inputLines[0];
    std::string objectHash = inputLines[1];
    if (objectName.back() == ':')
    {
      std::cout << prefix + objectName.substr(0, objectName.length() - 1) << "\n";
    }
    else
    {
      std::cout << prefix + objectName << "\n";
      if (objectName.substr(0, objectName.length() - 1) == ".parent" &&
          isRoot)
      {
        continue;
      }
      DirectoryOutput(objectHash, prefix + objectName, false);
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cerr << "Incorrect input. Try looking up README.md.\n"
              << "Usage: ./ls-oliger <dir> <hash>\n"
              << "Example: ./ls-oliger . 99682225f8eeb6a007bed389c124c659f596a794d556b4c865c99995c5f03b98\n";
    return 1;
  }
  std::string path = argv[1];
  std::string root = argv[2];
  // std::cout << "DEBUG | Main(" << path << "; " << root << ")\n";
  try
  {
    std::string hash = DirectorySearch(path, root);
    // std::cout << "DEBUG | DirectorySearch returned: " << hash << "\n";
    DirectoryOutput(hash, "", (hash == root));
  }
  catch (const std::exception &e)
  {
    std::cerr << "Caught an error during runtime. See the info below:\n"
              << e.what() << "\n";
    return 1;
  }
  return 0;
}