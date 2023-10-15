#include <iostream>
#include <algorithm>
#include <string>
#include <filesystem>
#include <fstream>

namespace {
    enum class obj_type {
        unknown = 0,
        file = 5,
        folder
    };


    enum class path_dir {
        parent = 0,
        child = 1
    };


    class FolderNode {
    private:
        std::shared_ptr<std::filesystem::path> full_path;
        std::filesystem::path::iterator it;

        explicit FolderNode(FolderNode* prev_node, path_dir dir)
                : full_path(prev_node->full_path)
                , it(prev_node->it)
        {
            if (dir == path_dir::child)
                ++it;
            else
                --it;
        }

    public:
        explicit FolderNode(const std::string& path)
                : full_path(std::make_shared<std::filesystem::path>(path))
                , it(full_path->begin()) {
        }

        std::string name() {
            return it->string();
        }

        bool empty() {
            return it == full_path->end();
        }

        FolderNode parent() {
            return FolderNode(this, path_dir::parent);
        }

        FolderNode child() {
            return FolderNode(this, path_dir::child);
        }

        obj_type get_type() {
            std::filesystem::path::iterator last = full_path->end();
            --last;
            if (it == last)
                return obj_type::file;
            return obj_type::folder;
        }

        friend std::ostream& operator<< (std::ostream& stream, FolderNode& node) {
            stream << node.name();
            return stream;
        }
    };


    struct line_info {
        obj_type type;
        std::string hash;
        std::string name;

        line_info()
                : type(obj_type::unknown)
        {
        }

        line_info(const std::string& line) {
            std::string separator_pattern = ":\t";
            auto line_separator_idx = line.find(separator_pattern);
            type = obj_type::file;
            if (line_separator_idx == std::string::npos) {
                separator_pattern = "/\t";
                line_separator_idx = line.find(separator_pattern);
                type = obj_type::folder;
            }

            if (line_separator_idx == std::string::npos || line_separator_idx == line.length() - separator_pattern.length() - 1) {
                type = obj_type::unknown;
                return;
            }

            name = line.substr(0, line_separator_idx);
            hash = line.substr(line_separator_idx + separator_pattern.length());
        }

        bool empty() const {
            return type == obj_type::unknown || hash.length() == 0 || name.length() == 0;
        }
    };
}

int main(int argc, char** argv) {
    if (argc != 3) {
        throw std::invalid_argument("Input arguments is invalid");
    }

    FolderNode current_node(argv[1]);
    std::string current_hash(argv[2]);
    std::string found_hash;

    while (!current_node.empty()) {
        found_hash.clear();
        std::ifstream fin{ current_hash };

        std::string current_line;
        while (getline(fin, current_line)) {
            line_info line(current_line);
            if (line.empty())
                throw std::invalid_argument("Found invalid line: " + current_line);

            if (line.name == current_node.name()) {
                found_hash = line.hash;
                break;
            }
        }

        if (found_hash.length() == 0) {
            std::string msg = "Can't find line with `" + current_node.name() + "` in " + current_hash;
            throw std::runtime_error(msg);
        }

        current_node = current_node.child();
        current_hash = found_hash;
    }

    std::string current_line;
    std::ifstream fin{ current_hash };
    while (getline(fin, current_line))
        std::cout << current_line << '\n';

    return 0;
}