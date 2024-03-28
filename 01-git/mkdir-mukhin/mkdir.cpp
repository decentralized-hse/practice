#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <openssl/sha.h>
#include <cstring>

void CheckError(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "Error: " << message << std::endl;
        exit(1);
    }
}

std::pair <std::string, std::string> ParseArgs(int argc, char *argv[]) {
    CheckError(argc == 3, "Usage: ./mkdir <path> <hash>");
    return {argv[1], argv[2]};
}

std::string Sha256(const std::string &data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);
    std::stringstream hexStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return hexStream.str();
}

std::string MakeDir(const std::string &path, const std::string &hash) {

    std::ifstream hashFile(hash);
    CheckError(hashFile.is_open(), "Failed to open file: " + hash);
    std::vector <std::string> list;
    std::string line;
    while (std::getline(hashFile, line)) {
        if (!line.empty()) {
            list.push_back(line);
        }
    }

    std::vector <std::string> filteredList;
    for (const auto &elem: list) {
        if (elem.find(".parent/") == std::string::npos && !elem.empty()) {
            filteredList.push_back(elem);
        }
    }
    list = filteredList;

    std::string newDirHash = Sha256("");
    std::ofstream newFile(newDirHash);
    CheckError(newFile.is_open(), "Failed to open file: " + hash);
    newFile << "";

    list.push_back(".parent/\t" + hash);

    std::vector <std::string> dirList;
    std::istringstream iss(path);
    std::string token;
    while (std::getline(iss, token, '/')) {
        dirList.push_back(token);
    }
    std::sort(list.begin(), list.end());
    std::string newDirData;
    for (size_t i = 0; i < list.size(); ++i) {
        if (!newDirData.empty()) {
            newDirData += "\n";
        }
        newDirData += list[i];
    }
    newDirData += "\n";
    std::string newRootHash = Sha256(newDirData);
    std::ofstream(newRootHash) << newDirData;
    return newRootHash;
}

int main(int argc, char *argv[]) {
    auto [path, hash] = ParseArgs(argc, argv);
    std::string resultHash = MakeDir(path, hash);
    std::cout << "New hash: " << resultHash << std::endl;
    return 0;
}
