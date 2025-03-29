#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <getopt.h>

using json = nlohmann::json;

struct Block {
    std::string hash;
    std::string prevBlockHash;
    unsigned long difficulty;
};

unsigned long calculateDifficulty(const std::string& difficultyTarget) {
    unsigned long zeroCount = 0;
    for (char c : difficultyTarget) {
        if (c == '0') {
            ++zeroCount;
        } else {
            break;
        }
    }
    return zeroCount;
}

// Прочитать блоки из базы
std::vector<Block> readBlocksFromDatabase(const std::string& dbPath) {
    std::vector<Block> blocks;
    
    for (const auto& entry : std::filesystem::directory_iterator(dbPath)) {
        if (entry.path().extension() == ".brik") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Unable to open file: " << entry.path() << std::endl;
                continue;
            }

            json blockJson;
            file >> blockJson;

            Block block;
            block.hash = blockJson.value("hash", "default_hash"); 
            block.prevBlockHash = blockJson.value("prevBlock", "");
            block.difficulty = calculateDifficulty(blockJson.value("difficultyTarget", ""));
            
            blocks.push_back(block);
        }
    }
    return blocks;
}

// Метод для поиска лучшей цепочки
Block getBestChain(const std::vector<Block>& blocks, bool maliciousMode) {
    std::map<std::string, unsigned long> chainDifficulties;
    
    for (const auto& block : blocks) {
        unsigned long totalDifficulty = block.difficulty;
        std::string currentHash = block.prevBlockHash;
        
        while (!currentHash.empty()) {
            auto it = std::find_if(blocks.begin(), blocks.end(), [&currentHash](const Block& blk) { return blk.hash == currentHash; });
            if (it != blocks.end()) {
                totalDifficulty += it->difficulty;
                currentHash = it->prevBlockHash;
            } else {
                break;
            }
        }
        
        chainDifficulties[block.hash] = totalDifficulty;
    }
    
    Block bestBlock;
    unsigned long targetDifficulty = maliciousMode ? ULONG_MAX : 0;
    for (const auto& entry : chainDifficulties) {
        const auto& hash = entry.first;
        unsigned long difficulty = entry.second;
        if ((maliciousMode && difficulty < targetDifficulty) || (!maliciousMode && difficulty > targetDifficulty)) {
            targetDifficulty = difficulty;
            auto it = std::find_if(blocks.begin(), blocks.end(), [&hash](const Block& blk) { return blk.hash == hash; });
            if (it != blocks.end()) {
                bestBlock = *it;
            }
        }
    }
    return bestBlock;
}

int main(int argc, char** argv) {
    std::string dbPath;
    bool maliciousMode = false;
    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"db", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"malicious", no_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "d:hm", long_options, &option_index)) != -1) {
        switch (c) {
            case 'd':
                dbPath = optarg;
                break;
            case 'm':
                maliciousMode = true;
                break;
            case 'h':
            default:
                std::cout << "Usage: " << argv[0] << " --db <path-to-db> [--malicious] [--help]\n";
                return 0;
        }
    }

    if (dbPath.empty()) {
        std::cerr << "Error: Database path is required. Use --help for usage.\n";
        return 1;
    }

    std::vector<Block> blocks = readBlocksFromDatabase(dbPath);
    if (blocks.empty()) {
        std::cerr << "No blocks found in the database.\n";
        return 1;
    }

    Block bestBlock = getBestChain(blocks, maliciousMode);
    std::cout << bestBlock.hash << std::endl;

    return 0;
}
