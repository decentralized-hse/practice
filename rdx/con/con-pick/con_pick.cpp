#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <getopt.h>

using json = nlohmann::json;

struct Block {
    std::string hash;
    std::string prevBlockHash;
    unsigned long difficulty; // Это можно вычислить из difficultyTarget
};

unsigned long calculateDifficulty(const std::string& difficultyTarget) {
    unsigned long zeroCount = 0;
    for (char c : difficultyTarget) {
        if (c == '0') {
            ++zeroCount;
        } else {
            break; // Останавливаемся, как только встречаем не 0
        }
    }
    return zeroCount;
}

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
            block.hash = entry.path().stem(); // Используем имя файла как hash для простоты
            block.prevBlockHash = blockJson.value("prevBlock", "");
            block.difficulty = calculateDifficulty(blockJson.value("difficultyTarget", ""));
            
            blocks.push_back(block);
        }
    }
    return blocks;
}

Block getBestChain(const std::vector<Block>& blocks) {
    Block bestBlock;
    unsigned long maxDifficulty = 0;
    for (const auto& block : blocks) {
        if (block.difficulty > maxDifficulty) {
            maxDifficulty = block.difficulty;
            bestBlock = block;
        }
    }
    return bestBlock;
}

int main(int argc, char** argv) {
    std::string dbPath;
    int option_index = 0;
    int c;

    // Опции для командной строки
    struct option long_options[] = {
        {"db", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "d:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'd':
                dbPath = optarg;
                break;
            case 'h':
            default:
                std::cout << "Usage: " << argv[0] << " --db <path-to-db> [--help]\n";
                return 0;
        }
    }

    if (dbPath.empty()) {
        std::cerr << "Error: Database path is required. Use --help for usage.\n";
        return 1;
    }

    // Логика работы с базой данных
    std::vector<Block> blocks = readBlocksFromDatabase(dbPath);
    if (blocks.empty()) {
        std::cerr << "No blocks found in the database.\n";
        return 1;
    }

    Block bestBlock = getBestChain(blocks);
    std::cout << "The best block hash is: " << bestBlock.hash << std::endl;

    return 0;
}
