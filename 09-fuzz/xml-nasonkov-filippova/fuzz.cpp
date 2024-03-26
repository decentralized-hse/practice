#include "xml-nasonkov.h"

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <stdint.h>
#include <cstddef>
#include <fstream>
#include <cassert>
#include <bitset>

#include <unistd.h>

const std::string file_name = "file_" + std::to_string(getpid());
const std::string bin_file_name = file_name + ".bin";
const std::string xml_file_name = file_name + ".xml";

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    // filename for parallel fuzzing
    // const std::string file_name = "file_" + std::to_string(getpid());
    // const std::string bin_file_name = file_name + ".bin";
    // const std::string xml_file_name = file_name + ".xml";

    const char* data_char = reinterpret_cast<const char*>(data);

    
    std::ofstream out_file(bin_file_name, std::ios::binary);
    out_file.write(data_char, size);
    out_file.close();

    if (!fromBinToXML(bin_file_name.c_str())) {
        // Inccorect data
        return 0;
    }

    fromXMLToBin(xml_file_name.c_str());

    std::ifstream res_file(bin_file_name, std::ios::in | std::ios::binary);
    std::vector<char> result_data(std::istreambuf_iterator<char>(res_file), {});
    
    if (result_data.size() != size || std::memcmp(result_data.data(), data_char, size) != 0) {
        res_file.close();
        std::cerr << "Fuzzing failed\n";

        assert(false);
        return 1;
    }
    res_file.close();
    return 0;
}