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

    std::vector<uint8_t> dataVector(data, data + size);

    // calling covertBinaryToJson
    TaskAux inputTask = getTaskType(&dataVector, nullptr, TaskAux::Type::BinaryToJson);
    std::string binToJson = covertBinaryToJson(inputTask);  // result will be writen into fuzzed_data.json

    // calling convertJsonToBinary
    TaskAux outputTask = getTaskType(nullptr, &binToJson, TaskAux::Type::JsonToBinary);
    std::vector<uint8_t> jsonToBin = convertJsonToBinary(outputTask);

    if (dataVector == jsonToBin) {
        return 0;
    } else {
        assert(false);
    }
}
