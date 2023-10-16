#pragma once

#include <string>
#include <filesystem>
#include "ObjType.h"


namespace {
    enum class PathDirection {
        parent = 0,
        child = 1
    };
}


class FolderNode {
private:
    std::shared_ptr <std::filesystem::path> full_path;
    std::filesystem::path::iterator it;

    explicit FolderNode(const FolderNode *prev_node, PathDirection dir)
            : full_path(prev_node->full_path), it(prev_node->it) {
        if (dir == PathDirection::child)
            ++it;
        else
            --it;
    }

public:
    explicit FolderNode(const std::string &path)
            : full_path(std::make_shared<std::filesystem::path>(path)), it(full_path->begin()) {
    }

    std::string name() const {
        return it->string();
    }

    std::string back() const {
        auto last = full_path->end();
        --last;
        return *last;
    }

    bool is_last() const {
        return it == full_path->end();
    }

    FolderNode parent() const {
        return FolderNode(this, PathDirection::parent);
    }

    FolderNode child() const {
        return FolderNode(this, PathDirection::child);
    }

    ObjType type() const {
        std::filesystem::path::iterator last = full_path->end();
        --last;
        if (it == last)
            return ObjType::file;
        return ObjType::folder;
    }

    friend std::ostream &operator<<(std::ostream &stream, FolderNode &node) {
        stream << node.name();
        return stream;
    }
};