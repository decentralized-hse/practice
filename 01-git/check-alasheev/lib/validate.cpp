#include "validate.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <vector>

#include "fmt/core.h"
#include "picosha2.h"

namespace {

std::string computeHash(const std::string& data) {
    return picosha2::hash256_hex_string(data);
}

bool isValidSHA256(const std::string& data) {
    if (data.size() != 2 * picosha2::k_digest_size) {
        return false;
    }

    return std::ranges::all_of(data, [](char c) {
        return std::isdigit(c) || ('a' <= c && c <= 'f');
    });
}

void validateHash(const std::string& path, const std::string& hash) {
    if (!isValidSHA256(hash)) {
        throw ValidationError(
            fmt::format("Directory or file hash is not valid: {}", hash));
    }

    std::ifstream f(path + hash);

    if (!f.good()) {
        throw ValidationError(
            fmt::format("No such file or directory: {}", hash));
    }

    std::stringstream ss;
    ss << f.rdbuf();
    if (computeHash(ss.str()) != hash) {
        throw ValidationError(
            fmt::format("Hash of file or directory is not correct: {}", hash));
    }
}

const std::regex DIRECTORY_LINE_REGEX(R"(^(\S+[:/])\t(\w+)$)");

std::map<std::string, std::string> parseDir(const std::string& path,
                                            const std::string& dir_hash) {
    std::map<std::string, std::string> hash_by_name;
    std::vector<std::string> lines;
    {
        std::ifstream dir{path + dir_hash};

        size_t line_num = 0;
        for (std::string line; std::getline(dir, line); ++line_num) {
            std::string name;
            std::string hash;

            if (!std::regex_match(line, DIRECTORY_LINE_REGEX)) {
                throw ValidationError(
                    fmt::format("line #{} don't match regex {} in directory {}",
                                line_num, R"(^(\S+[:/])\t(\w+)$)", dir_hash));
            }

            lines.push_back(line);
            std::istringstream ss{line};
            ss >> name >> hash;

            if (hash_by_name.count(name)) {
                throw ValidationError(fmt::format(
                    "line #{} contains duplicate name \"{}\" in directory {}",
                    line_num, name, dir_hash));
            }

            hash_by_name.emplace_hint(hash_by_name.end(), std::move(name),
                                      std::move(hash));
        }
    }

    auto sorted_lines = lines;
    sort(sorted_lines.begin(), sorted_lines.end());
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i] != sorted_lines[i]) {
            throw ValidationError(
                fmt::format("List is not sorted in directory {}", dir_hash));
        }
    }

    return hash_by_name;
}

void validateFile(const std::string& path, const std::string& hash) {
    validateHash(path, hash);
}

void validateDirectory(const std::string& path, const std::string& hash,
                       std::set<std::string>& checked_directories) {
    if (checked_directories.count(hash)) {
        return;
    }
    validateHash(path, hash);
    checked_directories.insert(hash);
    auto parsed_dir = parseDir(path, hash);

    for (auto [name, subhash] : parsed_dir) {
        if (name[name.size() - 1] == '/') {
            validateDirectory(path, subhash, checked_directories);
        } else {
            validateFile(path, subhash);
        }
    }
}

}  // namespace

void validateDirectory(const std::string& path, const std::string& hash) {
    std::set<std::string> checked_directories;
    validateDirectory(path, hash, checked_directories);
}
