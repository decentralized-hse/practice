#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct Block {
    std::string hash;
    std::string prevBlockHash;
    unsigned long difficulty; // Это можно вычислить из difficultyTarget
    // Дополнительные поля для других атрибутов блока
};

unsigned long calculateDifficulty(const std::string& difficultyTarget) {
    // Пример подсчета сложности — вы можете использовать реальную логику
    // Преобразовать target в число сложности, например, подсчитывая нули
    return static_cast<unsigned long>(difficultyTarget.length());
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
            // Прочитайте остальные необходимые поля, если это понадобиться
            
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

int main() {
    std::string dbPath = "../../db"; // Укажите правильный путь к вашей базе данных

    std::vector<Block> blocks = readBlocksFromDatabase(dbPath);

    if (blocks.empty()) {
        std::cout << "No blocks found in the database." << std::endl;
        return 1;
    }

    Block bestBlock = getBestChain(blocks);

    std::cout << "The best block hash is: " << bestBlock.hash << std::endl;

    return 0;
}
