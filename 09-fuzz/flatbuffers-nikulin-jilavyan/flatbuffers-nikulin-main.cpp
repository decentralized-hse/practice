#include "flatbuffers-nikulin.h"

#include <iostream>
#include <string>
#include <utility>

using namespace std;

pair<string, string> parseInputFilename(const string& inputName) {
    auto last_separator = inputName.find_last_of(".");
    return {inputName.substr(0, last_separator), inputName.substr(last_separator + 1)};
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: file to process" << endl;
        return 1;
    }
    auto [filename, format] = parseInputFilename(argv[1]);
    if (format == "bin") {
        if (!convertToFlatbuffer(filename)) {
            cerr << "Malformed input\n";
            return -1;
        }
    } else if (format == "flat") {
        convertToBin(filename);
    } else {
        cout << "File format should be either flat or bin. Got " << format << endl;
        return 1;
    }
    return 0;
}
