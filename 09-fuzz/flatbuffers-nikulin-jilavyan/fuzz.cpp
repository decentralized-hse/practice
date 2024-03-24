#include "flatbuffers-nikulin.h"

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

#include <unistd.h>

namespace {
    const std::string fileName = "tmp" + std::to_string(getpid());
    const std::string binFileName = fileName + ".bin";
    const std::string fbsFileName = fileName + ".flat";

    std::vector<char> readFileIntoVec(const std::string& fileName) {
        std::ifstream file(fileName, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
}


extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data_u8, size_t Size) {
    const char* Data = reinterpret_cast<const char*>(Data_u8);

    {
        std::ofstream binFile(binFileName, std::ios::binary);
        binFile.write(Data, Size);
    }

    if (!convertToFlatbuffer(fileName)) {
        return 0;
    }

    convertToBin(fileName);

    auto data = readFileIntoVec(binFileName);
    if (data.size() != Size || std::memcmp(data.data(), Data, Size) != 0) {
        std::cerr << "Round-trip guarantee violation\n";
        return -1;
    }

    return 0;
}
