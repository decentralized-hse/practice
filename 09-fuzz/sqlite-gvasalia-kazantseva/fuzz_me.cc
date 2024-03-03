#include <stdint.h>
#include <cstddef>
#include <fstream>
#include <cassert>
#include <bitset>

#include "sqlite-gvasalia-kazantseva.h"
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

    std::string fileName = "fuzzed_data.bin";
    ConvertBinaryDataIntoSqlite(fileName.c_str());
    fileName = "fuzzed_data.sqlite";
    std::string newFileName = "converted_data.bin";
    ConvertSqliteDataIntoBinary(fileName.c_str(), newFileName.c_str());


    // Compare the buffer with original data
    std::ifstream inFile("converted_data.bin", std::ios::in | std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(inFile), {});
    bool isSame = (memcmp(data, buffer.data(), size) == 0);
    if(isSame) {
        inFile.close();
        return 0;
    } else {
        assert(false);
    }
}