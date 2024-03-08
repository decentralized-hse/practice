#pragma once

#include <string>

#include "constants.h"

enum class ObjectType {
    unknown = 0,
	dir = 2,
    file = 1
};

struct ObjectInfo {
	ObjectType Type = ObjectType::unknown;
	std::string Name;
	std::string Hash;

	ObjectInfo(const ObjectType& type, const std::string& name, const std::string& hash)
	: Type(type)
	, Name(name)
	, Hash(hash) {}
};

ObjectInfo getObjectInfo(const std::string& currentLine) {
    size_t splitPos = 0;
    if ((splitPos = currentLine.find(DIR_SEPARATOR)) != std::string::npos) {
        std::string dirName = currentLine.substr(0, splitPos) + "/";
        std::string hashTo = currentLine;
        hashTo.erase(0, splitPos + DIR_SEPARATOR.length());
        return ObjectInfo(ObjectType::dir, dirName, hashTo);
    } else if ((splitPos = currentLine.find(FILE_SEPARATOR)) != std::string::npos) {
        std::string fileName = currentLine.substr(0, splitPos);
        std::string hashTo = currentLine;
        hashTo.erase(0, splitPos + FILE_SEPARATOR.length());
        return ObjectInfo(ObjectType::file, fileName, hashTo);
    }

    std::string errorMessage = "Wrong expression: " + currentLine;
    throw std::runtime_error(errorMessage);
}

