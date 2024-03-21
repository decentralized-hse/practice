#include "xml-nasonkov.h"

#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <cctype>
#include <cstdlib>

char bin_extension[4] = "bin";
char xml_extension[4] = "xml";

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Expected filename as command line argument!\n";
        return 1;
    }
    char* filename = argv[1];
    size_t filename_len = std::strlen(filename);
    if (filename_len < 4 || 
        (std::strcmp(filename + filename_len - 3, bin_extension) != 0 &&
         std::strcmp(filename + filename_len - 3, xml_extension) != 0)) {
        std::cerr << "Invalid file extension!\n";
        return 1;
    }
    if (std::strcmp(filename + filename_len - 3, bin_extension) == 0) {
        fromBinToXML(filename);
    } else {
        fromXMLToBin(filename);
    }
    return 0;
}