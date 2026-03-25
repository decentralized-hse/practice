#include "../src/bason_codec.h"
#include "../src/json_converter.h"
#include "../src/flatten.h"
#include "../src/strictness.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstring>

////////////////////////////////////////////////////////////////////////////////

using namespace NBason;

////////////////////////////////////////////////////////////////////////////////

void PrintUsage(const char* progName)
{
    std::cerr << "Usage: " << progName << " [options] <file>\n"
              << "\nOptions:\n"
              << "  --hex           Show hex dump with annotations\n"
              << "  --json          Convert to JSON\n"
              << "  --validate=N    Validate with strictness mask N (hex)\n"
              << "  --flatten       Convert nested to flat\n"
              << "  --unflatten     Convert flat to nested\n"
              << "  --help          Show this help\n";
}

void PrintHexDump(const std::vector<uint8_t>& data)
{
    std::cout << "=== Hex Dump ===\n";
    
    size_t offset = 0;
    while (offset < data.size()) {
        try {
            auto [record, size] = DecodeBason(&data[offset], data.size() - offset);
            
            // Print offset
            std::cout << std::setfill('0') << std::setw(8) << std::hex << offset << ": ";
            
            // Print hex bytes
            for (size_t i = 0; i < size && i < 32; ++i) {
                std::cout << std::setw(2) << std::hex << (int)data[offset + i] << " ";
            }
            if (size > 32) {
                std::cout << "...";
            }
            std::cout << std::dec << "\n";
            
            // Print annotation
            std::cout << "  Type: ";
            switch (record.Type) {
                case EBasonType::Boolean: std::cout << "Boolean"; break;
                case EBasonType::Array:   std::cout << "Array"; break;
                case EBasonType::String:  std::cout << "String"; break;
                case EBasonType::Object:  std::cout << "Object"; break;
                case EBasonType::Number:  std::cout << "Number"; break;
            }
            
            std::cout << ", Key: \"" << record.Key << "\""
                      << ", Value: \"" << record.Value << "\""
                      << ", Size: " << size << " bytes\n\n";
            
            offset += size;
        } catch (const std::exception& e) {
            std::cerr << "Error at offset " << offset << ": " << e.what() << "\n";
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    std::string filename;
    bool showHex = false;
    bool convertJson = false;
    bool doFlatten = false;
    bool doUnflatten = false;
    uint16_t validateMask = 0;
    bool doValidate = false;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--hex") {
            showHex = true;
        } else if (arg == "--json") {
            convertJson = true;
        } else if (arg == "--flatten") {
            doFlatten = true;
        } else if (arg == "--unflatten") {
            doUnflatten = true;
        } else if (arg.substr(0, 11) == "--validate=") {
            doValidate = true;
            validateMask = std::stoi(arg.substr(11), nullptr, 16);
        } else if (arg[0] != '-') {
            filename = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    if (filename.empty()) {
        std::cerr << "Error: No input file specified\n";
        PrintUsage(argv[0]);
        return 1;
    }
    
    // Read file
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file: " << filename << "\n";
        return 1;
    }
    
    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    
    if (data.empty()) {
        std::cerr << "Error: File is empty\n";
        return 1;
    }
    
    try {
        if (showHex) {
            PrintHexDump(data);
        } else if (convertJson) {
            std::string json = BasonToJson(data.data(), data.size());
            std::cout << json << "\n";
        } else if (doFlatten) {
            auto records = DecodeBasonAll(data.data(), data.size());
            for (const auto& record : records) {
                auto flat = FlattenBason(record);
                for (const auto& flatRecord : flat) {
                    auto encoded = EncodeBason(flatRecord);
                    std::cout.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
                }
            }
        } else if (doUnflatten) {
            auto records = DecodeBasonAll(data.data(), data.size());
            auto nested = UnflattenBason(records);
            auto encoded = EncodeBason(nested);
            std::cout.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
        } else if (doValidate) {
            auto records = DecodeBasonAll(data.data(), data.size());
            bool allValid = true;
            for (const auto& record : records) {
                if (!ValidateBason(record, validateMask)) {
                    allValid = false;
                    std::cerr << "Validation failed for record with key: " << record.Key << "\n";
                }
            }
            if (allValid) {
                std::cout << "All records valid\n";
                return 0;
            } else {
                return 1;
            }
        } else {
            // Default: just decode and print basic info
            auto records = DecodeBasonAll(data.data(), data.size());
            std::cout << "Found " << records.size() << " records\n";
            for (size_t i = 0; i < records.size(); ++i) {
                std::cout << "Record " << i << ": key=\"" << records[i].Key << "\"\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
