#include <iostream>
#include <fstream>

#include "FolderNode.h"
#include "LineInfo.h"
#include "Sha256HashChecker.h"


int main(int argc, char **argv) {
    if (argc != 3) {
        throw std::invalid_argument("Input arguments is invalid");
    }

    FolderNode current_node(argv[1]);
    LineInfo current_line("`root`", argv[2], current_node.type());

    while (!current_node.is_last()) {
        std::string prev_hash = current_line.hash;
        std::stringstream file_buffer{};
        std::ifstream fin{current_line.hash};
        file_buffer << fin.rdbuf();
        fin.close();

        uint32_t line_number = 0;
        std::string read_line;
        while (getline(file_buffer, read_line)) {
            ++line_number;
            current_line = LineInfo(read_line);
            if (current_line.empty()) {
                std::cerr << "Found invalid line number: " << line_number << '\n';
                continue;
            }

            if (current_line.name == current_node.name())
                break;
        }

        if (!Sha256HashChecker::is_valid(file_buffer.str(), prev_hash))
            throw std::runtime_error("ERROR!!! " + current_line.name + " file has been compromised");

        if (current_line.empty())
            throw std::runtime_error("Can't find line with file `" + current_node.name() + "` in " + prev_hash);

        current_node = current_node.child();
    }

    std::ifstream fin{current_line.hash};

    std::stringstream buffer{};
    buffer << fin.rdbuf();

    std::string content{buffer.str()};

    if (!Sha256HashChecker::is_valid(content, current_line.hash))
        throw std::runtime_error("ERROR!!! " + current_line.name + " file has been compromised");

    std::cout << content;

    return 0;
}