#include <iostream>

#include "lib/validate.hpp"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Format: " << argv[0] << "<path> <root-hash>";
        return 1;
    }

    std::string path = argv[1];
    if (path[path.size() - 1] != '/') {
        path.push_back('/');
    }

    std::cout << "Validation result:\n";
    try {
        validateDirectory(argv[1], argv[2]);
    } catch (ValidationError& err) {
        std::cout << err.what() << '\n';
        return 0;
    }

    std::cout << "OK\n";
}
