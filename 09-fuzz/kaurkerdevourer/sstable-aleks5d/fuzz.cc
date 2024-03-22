#include <stdint.h>
#include <cstddef>
#include <fstream>
#include <cassert>
#include <bitset>

#include "convert.h"
#include "precheck.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // generated fuzzed data check
    bool isDataCorrect = CheckData(data, size);
    if (!isDataCorrect) {
        return 0;
    }

    // generated fuzzed data write into file
    std::ofstream outFile("fuzzed_data.bin", std::ios::out | std::ios::binary);
    outFile.write((char*)data, size);
    outFile.close();

    std::string fileName = "fuzzed_data";
    parseBin(fileName);
    fileName = "fuzzed_data";
    std::string newFileName = "fuzzed_data";
    parseSstable(fileName, newFileName);

    // Compare the buffer with original data
   std::ifstream inFile("fuzzed_data_output.bin", std::ios::in | std::ios::binary);
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    bool isSame = (memcmp(data, buffer.str().data(), size) == 0);
    if(isSame) {
        inFile.close();
        return 0;
    } else {
        assert(false);
    }
}