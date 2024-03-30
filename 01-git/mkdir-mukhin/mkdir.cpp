#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <openssl/sha.h>
#include <cstring>

std::string Sha256(const std::string &data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);
    std::stringstream hexStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return hexStream.str();
}

std::string makeBlob(const std::string &data) {
    std::string hash = Sha256(data);
    std::ofstream out(hash, std::ofstream::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing" << std::endl;
        exit(1);
    }
    out << data;
    return hash;
}

std::vector<std::string> getList(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading" << std::endl;
        exit(1);
    }

    std::vector<std::string> resList;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            resList.push_back(line);
        }
    }
    return resList;
}

std::pair<std::string, std::string> ParseArgs(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Wrong input: ./mkdir <path> <hash>";
        exit(1);
    }
    return {argv[1], argv[2]};
}

std::vector<std::string> SplitPath(const std::string &path) {
    std::vector<std::string> list;
    size_t start = 0;
    size_t end = path.find("/");
    while (end != std::string::npos) {
        list.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find("/", start);
    }
    list.push_back(path.substr(start, end));
    return list;
}

std::string CreateData(std::vector<std::string> &list) {
    std::sort(list.begin(), list.end());
    std::string data;
    for (const auto &item: list) {
        data += item + "\n";
    }
    return data;
}

std::string CreateNewHash(std::vector<std::string> &list) {
    std::string newData = CreateData(list);
    std::string newHash = Sha256(newData);
    std::ofstream(newHash) << newData;
    return newHash;
}

std::string UpdateHash(std::vector<std::string> &path, std::vector<std::string> &dirList,
                       std::string hashToUpd, const std::string &hash) {
    int updIndex = dirList.size() - 1;
    std::string updDir = dirList[updIndex];
    for (int i = path.size() - 1; i >= 0; --i) {
        auto lastList = getList(path[i]);
        bool updated = false;
        for (size_t j = 0; j < lastList.size(); ++j) {
            if (lastList[j].find(updDir) != std::string::npos) {
                lastList[j] = updDir + "/\t" + hashToUpd;
                updated = true;
            }
            if (lastList[j].find(".parent/") != std::string::npos && i == 0) {
                lastList[j] = ".parent/\t" + hash;
            }
        }
        if (!updated) {
            lastList.push_back(updDir + "/\t" + hashToUpd);
        }

        std::string data = CreateData(lastList);
        hashToUpd = makeBlob(data);

        --updIndex;
        if (updIndex >= 0) {
            updDir = dirList[updIndex];
        }
    }
    return hashToUpd;
}

std::string MakeDir(const std::string &path, const std::string &hash) {
    auto list = getList(hash);
    std::vector<std::string> clearList;
    for (const auto &elem: list) {
        if (elem.find(".parent/") == std::string::npos && !elem.empty()) {
            clearList.push_back(elem);
        }
    }
    list = std::move(clearList);

    std::string newHash = makeBlob("");
    list.push_back(".parent/\t" + hash);

    std::vector<std::string> fullPath;
    auto dirList = SplitPath(path);

    if (path.find("/") == std::string::npos) {
        list.push_back(path + "/\t" + newHash);
    } else {
        std::string parentDir = hash;
        for (size_t i = 0; i < dirList.size(); ++i) {
            fullPath.push_back(parentDir);
            if (i + 1 == dirList.size()) {
                std::fstream f(parentDir, std::fstream::app | std::fstream::out);
                if (!f.is_open()) {
                    std::cerr << "File open failed" << std::endl;
                    exit(1);
                }
            } else {
                std::ifstream file(parentDir);
                if (!file.is_open()) {
                    std::cerr << "File open failed" << std::endl;
                    exit(1);
                }
                std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                if (data.find(dirList[i]) != std::string::npos) {
                    auto parentList = getList(parentDir);
                    bool found = false;
                    for (const auto &f: parentList) {
                        if (f.find(dirList[i]) != std::string::npos) {
                            parentDir = f.substr(f.find("\t") + 1);
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        continue;
                    }

                    std::cerr << "Incorrect structure\n";
                    exit(1);
                } else {
                    std::cerr << dirList[i] << " doesn't exist\n";
                    exit(1);
                }
            }
        }
    }

    if (fullPath.size() <= 1) {
        return CreateNewHash(list);
    }
    return UpdateHash(fullPath, dirList, newHash, hash);
}

int main(int argc, char *argv[]) {
    auto [path, hash] = ParseArgs(argc, argv);
    std::string resultHash = MakeDir(path, hash);
    std::cout << "New hash: " << resultHash << std::endl;
    return 0;
}