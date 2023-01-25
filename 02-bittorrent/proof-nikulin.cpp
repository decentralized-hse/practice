#include <iostream>
#include <string>
#include <vector>

using namespace std;

vector<string> readFile() {
    vector<string> res;
    string current;
    while (cin >> current) {
        res.push_back(current);
    }
    return res;
}

vector<string> process(vector<string> hashes, int ind) {
    int position = 2 * ind;
    int maxpos = position;
    vector<string> res;
    int lvl = 0;
    while (maxpos < hashes.size()) {
        res.push_back(hashes[position]);
        ++lvl;
        int oddity = position / (1 << lvl);
        int opposite = (oddity % 2 ? position - (1 << lvl) : position + (1 << lvl));
        position = (position + opposite) / 2;
        maxpos = position + (1 << lvl) - 1;
    }
    return res;
}

//input file - hashtree
int main(int argc, char** argv) {
    if (argc != 3) {
        return 1;
    }
    string inName(argv[1]);
    string inChunk(argv[2]);
    freopen(argv[1], "r", stdin);
    vector<string> hashes = readFile();
    vector<string> out = process(hashes, stoi(inChunk));
    string outName = inName + "." + inChunk + ".proof";
    freopen(outName.c_str(), "w", stdout);
    for (const string& cur : out) {
        cout << cur << "\n";
    }
    return 0;
}
