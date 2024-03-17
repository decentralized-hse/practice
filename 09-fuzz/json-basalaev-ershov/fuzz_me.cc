#include <stdint.h>
#include <cstddef>
#include <fstream>
#include <cassert>
#include <bitset>
#include <string>
#include <cstdio>

#include "json-basalaev.h"
#include "precheck.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    // generated fuzzed data check
    bool isDataCorrect = CheckData(data, size);
    if (!isDataCorrect) {
        return 0;
    }

    // generated fuzzed data write into file
    const std::string inputFileName = "fuzzed_data.bin";
    std::ofstream outFile(inputFileName, std::ios::out | std::ios::binary);
    outFile.write((char*)data, size);
    outFile.close();

    // calling covertBinaryToJson
    TaskAux inputTask = getTaskType(inputFileName);
    covertBinaryToJson(inputTask);  // result will be writen into fuzzed_data.json

    // calling convertJsonToBinary
    const std::string outputFileName = "fuzzed_data.json";
    TaskAux outputTask = getTaskType(outputFileName);
    convertJsonToBinary(outputTask); 

    // Compare the buffer with original data
    std::ifstream inFile(inputFileName, std::ios::in | std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(inFile), {});
    bool isSame = (memcmp(data, buffer.data(), size) == 0);
    if(isSame) {
        inFile.close();
        return 0;
    } else {
        assert(false);
    }
}
