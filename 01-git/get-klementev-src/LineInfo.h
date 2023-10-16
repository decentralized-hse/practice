#pragma once

#include <string>
#include "ObjType.h"


struct LineInfo {
    ObjType type;
    std::string hash;
    std::string name;

    LineInfo()
            : type(ObjType::unknown) {
    }

    LineInfo(std::string name, std::string hash, ObjType type)
            : name{std::move(name)}, hash{std::move(hash)}, type{type} {
    }

    explicit LineInfo(const std::string &line) {
        std::string separator_pattern = ":\t";
        auto line_separator_idx = line.find(separator_pattern);
        type = ObjType::file;
        if (line_separator_idx == std::string::npos) {
            separator_pattern = "/\t";
            line_separator_idx = line.find(separator_pattern);
            type = ObjType::folder;
        }

        if (line_separator_idx == std::string::npos ||
            line_separator_idx == line.length() - separator_pattern.length() - 1) {
            type = ObjType::unknown;
            return;
        }

        name = line.substr(0, line_separator_idx);
        hash = line.substr(line_separator_idx + separator_pattern.length());
    }

    bool empty() const {
        return type == ObjType::unknown || hash.length() == 0 || name.length() == 0;
    }
};
