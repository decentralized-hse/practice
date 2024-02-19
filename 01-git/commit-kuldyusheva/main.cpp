#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "openssl/sha.h"

std::string PATH = "./";
std::string COMMIT_PREFIX = ".commit:\t";
std::string PARENT_PREFIX = ".parent:\t";

std::string TMP_COMMIT_FILE = "tmp_commit";
std::string TMP_ROOT_FILE = "tmp_root";


std::string sha256(std::string path) {
    std::ifstream ofs(path, std::ios::binary);
    std::ostringstream oss;
    oss << ofs.rdbuf();
    std::string str = oss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    const unsigned char* data = (const unsigned char*) str.c_str();
    SHA256(data, str.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string CreateCommitFile(std::string rootHash, std::string comment) {
    std::ofstream tmp(PATH + TMP_COMMIT_FILE);
    tmp << "Root:\t" << rootHash << "\n";

    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    tmp << "Date:\t" << std::put_time(&tm, "%d %b %Y %T %Z") << "\n";

    tmp << "\n" << comment << "\n";

    tmp.close();
    std::string hash = sha256(PATH + TMP_COMMIT_FILE);
    std::filesystem::rename(PATH + TMP_COMMIT_FILE, PATH + hash);

    return hash;
}

std::string CreateNewRootFile(std::string rootHash, std::string commitHash) {
    std::ifstream file(PATH + rootHash);

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        if (line.substr(0, COMMIT_PREFIX.size()) != COMMIT_PREFIX &&
            line.substr(0, PARENT_PREFIX.size()) != PARENT_PREFIX)
        {
            lines.push_back(line);
        }
    }

    std::string commitLine = COMMIT_PREFIX + commitHash;
    lines.push_back(commitLine);
    std::string parentLine = PARENT_PREFIX + rootHash;
    lines.push_back(parentLine);

    std::sort(lines.begin(), lines.end());

    std::ofstream tmp(PATH + TMP_ROOT_FILE);
    for (std::string line : lines) {
        tmp << line << "\n";
    }
    tmp.close();
    std::string hash = sha256(PATH + TMP_ROOT_FILE);
    std::filesystem::rename(PATH + TMP_ROOT_FILE, PATH + hash);
    return hash;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Invalid arguments. Expected <hash root>." << std::endl;
        exit(1);
    }

    std::string rootHash(argv[1]);
    if (!std::filesystem::exists(rootHash)) {
        std::cerr << "Invalid hash root. No such file: " << rootHash << std::endl;
        exit(1);
    }

    std::string comment;
    std::cout << "Write comment for commit" << std::endl;
    std::cin >> comment;

    std::string commitHash = CreateCommitFile(rootHash, comment);

    std::string newRootHash = CreateNewRootFile(rootHash, commitHash);
    std::cout << newRootHash << "\n";
}
