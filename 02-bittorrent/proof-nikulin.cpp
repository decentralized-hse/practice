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
        ++lvl;
        int oldPosition = position;
        position = (position + (position ^ (1 << lvl))) / 2;
        maxpos = position + (1 << lvl) - 1;
        if (maxpos < hashes.size())
            res.push_back(hashes[oldPosition ^ (1 << lvl)]);
    }
    return res;
}

//input file - hashtree
int main(int argc, char** argv) {
    if (argc != 3) {
	cout << "Usage: path to file with a hashtree, chunk number" << endl;
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
