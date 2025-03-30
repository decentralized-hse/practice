#include "con_pick.h"
#include <iostream>
#include <getopt.h>

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
