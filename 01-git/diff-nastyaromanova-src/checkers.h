#pragma once

#include "constants.h"

bool isValidPath(std::string path) {
    if (path.find(" ") != std::string::npos ||  path.find('\t') != std::string::npos || path.find(':') != std::string::npos) {
        return false;
    }
    return true;
}

void checkArguments(int argc, char **argv, std::string& path, std::string& argHashFrom, std::string& argHashTo) {
    if (argc != 4) {
        throw std::invalid_argument("Incorrect number of arguments");
    }

    path = argv[1], argHashFrom = argv[2], argHashTo = argv[3];

    if (!isValidPath(path)) {
        throw std::invalid_argument("Invalid path");
    }
    if (argHashFrom.size() != HASH_SIZE || argHashTo.size() != HASH_SIZE) {
        throw std::invalid_argument("Invalid SHA-256 hash");
    }
}