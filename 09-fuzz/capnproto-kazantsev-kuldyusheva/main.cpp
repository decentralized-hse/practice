#include <string.h>

#include "validate.h"
#include "capnproto-kazantsev-kuldyusheva.c++"

std::pair<std::string, std::string> parseFileName(const std::string& filename) {
    if (filename.empty()) {
        throw std::runtime_error("Empty filename");
    }

    size_t pos = filename.find_last_of(".");
    std::string name = filename.substr(0, pos);
    std::string extension = filename.substr(pos);

    return {name, extension};
}  

int main(int argc, char *argv[]) {
    if (argc != 2 || !argv[1]) {
        throw std::runtime_error("Wrong arguments count");
    }

    std::string filename = argv[1];
    auto [name, extension] = parseFileName(filename);

    if (extension == ".capnproto") {
        FromCapnprotoToStudent(name);
    } else if (extension == ".bin") {
        if (!validateData(name)) {
            throw std::runtime_error("Invalid binary data");
        }
        FromStudentToCapnproto(name);
    } else {
        throw std::runtime_error("Unknown file extension");
    }

    return 0;
}
