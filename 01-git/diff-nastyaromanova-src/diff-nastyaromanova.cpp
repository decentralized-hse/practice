#include <cstddef>
#include <iostream>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include "constants.h"
#include "checkers.h"
#include "object_info.h"

void openFile(std::ifstream& file, const std::string& fileName) {
    try {
        file.open(fileName);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

void moveByPath(const std::string& path, const std::string& hash, std::string& resultHash) {
    if (path == ".") {
        resultHash = hash;
        return;
    }

    std::string currentObjectForSearch, newPath;
    size_t splitPos = 0;
    if ((splitPos = path.find("/")) != std::string::npos) {
        currentObjectForSearch = path.substr(0, splitPos) + "/";
        newPath.erase(0, splitPos + 1);
    } else {
        currentObjectForSearch = path;
        newPath = "";
    }

    std::ifstream file;
    openFile(file, hash);

    if (file.is_open()) {
        std::string currentLine;
        while (std::getline(file, currentLine)) {
            ObjectInfo objectInfo = getObjectInfo(currentLine);
            if (objectInfo.Name == currentObjectForSearch) {
                if (objectInfo.Type == ObjectType::file) {
                    throw std::runtime_error("Wrong path. It must be dir");
                }

                if (objectInfo.Type == ObjectType::dir) {
                    if (!newPath.empty()) {
                        moveByPath(newPath, objectInfo.Hash, resultHash);
                    } else {
                        resultHash = objectInfo.Hash;
                        return;
                    }
                }
            }
        }
    }
}

void dfsReading(const std::string& hash,
                std::unordered_map<std::string, std::unordered_set<std::string> >& objectToHashes,
                std::unordered_map<std::string, std::unordered_set<std::string> >& hashesToObjects,
                const std::string& prefix = "") {
    std::ifstream file;
    openFile(file, hash);

    if (file.is_open()) {
        std::string currentLine;
        while (std::getline(file, currentLine)) {
            ObjectInfo objectInfo = getObjectInfo(currentLine);
            objectToHashes[prefix + objectInfo.Name].insert(objectInfo.Hash);
            hashesToObjects[objectInfo.Hash].insert(prefix + objectInfo.Name);

            if (objectInfo.Type == ObjectType::dir) {
                dfsReading(objectInfo.Hash, objectToHashes, hashesToObjects, objectInfo.Name);
            }
        }
    } else {
        return;
    }
}

int main(int argc, char **argv) {
    std::string path, argHashFrom, argHashTo;
    checkArguments(argc, argv, path, argHashFrom, argHashTo);

    std::string hashTo, hashFrom;
    moveByPath(path, argHashFrom, hashFrom);
    moveByPath(path, argHashTo, hashTo);

    std::unordered_map<std::string, std::unordered_set<std::string> > objectToHashesFrom;
    std::unordered_map<std::string, std::unordered_set<std::string> > hashesToObjectsFrom;
    dfsReading(hashFrom, objectToHashesFrom, hashesToObjectsFrom);

    std::unordered_map<std::string, std::unordered_set<std::string> > objectToHashesTo;
    std::unordered_map<std::string, std::unordered_set<std::string> > hashesToObjectsTo;
    dfsReading(hashTo, objectToHashesTo, hashesToObjectsTo);
    
    for (const auto& [objectName, objectHashes] : objectToHashesFrom) {
        if (!objectToHashesTo.contains(objectName)) {
            std::cout << "-\t" << objectName << std::endl;
        } else if (objectName.back() == '/') {
            continue;
        } else if (objectToHashesFrom[objectName] != objectToHashesTo[objectName]) {
            std::cout << "d\t" << objectName << std::endl;
        }
    }

    for (const auto& [objectName, objectHashes] : objectToHashesTo) {
        if (!objectToHashesFrom.contains(objectName)) {
            std::cout << "+\t" << objectName << std::endl;
        }
    }
    return 0;
}