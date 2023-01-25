#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

vector<string> getHashTree(const string& filePath) {
    string hashTreeFilePath = filePath + ".hashtree";

    ifstream in(hashTreeFilePath);
    cout << "reading " << hashTreeFilePath << "..." << endl;

    vector<string> hashTree;
    string hashBlock;
    while (getline(in, hashBlock)) {
        hashTree.emplace_back(move(hashBlock));
    }

    return hashTree;
}

vector<string> getPeaks(const vector<string>& hashTree) {
    const static int maxBits = 32;
    const static int hashSize = 64;
    const static string zero = string(hashSize, '0');

    vector<string> peaks(maxBits, zero);
    size_t blockStart = 0;
    for (long long i = maxBits - 1; i > 0; --i) {
        size_t currentBlock = (1 << i) - 1;
        if (blockStart + currentBlock <= hashTree.size()) {
            size_t halfBlock = currentBlock / 2;
            peaks[i - 1] = hashTree[blockStart + halfBlock];
            blockStart += currentBlock;
        }
    }

    return peaks;
}

void printPeaks(const string& filePath, const vector<string>& peaks) {
    string peaksFilePath = filePath + ".peaks";

    ofstream out(peaksFilePath);
    cout << "putting the peaks into " << peaksFilePath << "..." << endl;

    for (auto& peak: peaks) {
        out << peak << "\n";
    }
}

void printPeaksFromHashTree(const string& dataFilePath) {
    auto hashTree = getHashTree(dataFilePath);
    auto peaks = getPeaks(hashTree);
    printPeaks(dataFilePath, peaks);
    cout << "all done!" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Error. Add path of file with a hashtree in the code parameters" << endl;
        return 1;
    }

    printPeaksFromHashTree(argv[1]);
    return 0;
}