#include <iostream>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <vector>

std::vector<std::string> readRootFile(std::string& path) {
    std::ifstream file(path);
    std::string line = "";
    std::vector<std::string> commitFiles;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string item1 = "";
        std::string item2 = "";
        if (!(iss >> item1 >> item2)) {
            std::cerr << "Parse error!\n";
            exit(1);
        }

        if (item1 == ".commit:") {
            commitFiles.push_back(item2);
        }

    }
    file.close();
    return commitFiles;
}


std::string getNewRoot(std::string& line) {
    if (line.find("Root") != std::string::npos) {
        std::istringstream iss(line);
        std::string item1 = "";
        std::string item2 = "";
        iss >> item1 >> item2;
        return item2;
    }
    return "";
}


std::string printCommitFile(std::string& path) {
    std::ifstream file(path);

    std::string nextRootFile = "";
    std::string line = "";
    std::string subString = "Root";
    std::getline(file, line);

    if (line.compare(0, subString.size(), subString) != 0) {
        return nextRootFile;
    } else {
        std::cout << line << "\n";
        std::istringstream iss(line);
        std::string item1 = "";
        std::string item2 = "";
        iss >> item1 >> item2;
        nextRootFile = item2;
    }

    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }
    return nextRootFile;
}


std::string getNoCommitRoot(std::string& getNoCommitRoot) {
    std::string line = "";

    std::ifstream file(getNoCommitRoot);
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string item1 = "";
        std::string item2 = "";
        iss >> item1 >> item2;

        if (item1 == ".parent/") {
            return item2;
        }
    }
    return "";
}


int main(int argc, char* argv[]) {
    if (argc < 1) {
        std::cerr << "Current hash wasn't provided. Exit program\n";
        return 0;
    }
    std::string currentRootFile = argv[1];
    std::string currentRootFileNoCommit = argv[1];

    std::string path = std::filesystem::current_path();
    std::vector<std::string> fileNames;
    for(const auto & entry : std::filesystem::directory_iterator(path)) {
        fileNames.push_back(entry.path());
    }

    while (currentRootFile != "") {
        std::string currentRootFile = "";
        for (const auto& fileName : fileNames) {
            if (fileName.find(currentRootFileNoCommit) != std::string::npos) {
                currentRootFile = fileName;
            }
        }

        if (currentRootFile == "") {
            return 0;
        }

        std::vector<std::string> currentCommitFiles = readRootFile(currentRootFile);
        currentRootFile = "";
        for (auto& currentCommitFile: currentCommitFiles) {
            std::string fullPath = path + "/" + currentCommitFile;
            if (printCommitFile(fullPath) != "") {
                break;
            }
        }

        currentRootFile = getNoCommitRoot(currentRootFileNoCommit);
        currentRootFileNoCommit = currentRootFile;

        if (currentRootFile == "") {
            return 0;
        }
    }
}
