#ifndef CON_PICK_H
#define CON_PICK_H

#include <string>
#include <vector>

struct Block {
    std::string hash;
    std::string prevBlockHash;
    unsigned long difficulty;
};

unsigned long calculateDifficulty(const std::string& difficultyTarget);
std::vector<Block> readBlocksFromDatabase(const std::string& dbPath);
Block getBestChain(const std::vector<Block>& blocks, bool maliciousMode);

#endif // CON_PICK_H
