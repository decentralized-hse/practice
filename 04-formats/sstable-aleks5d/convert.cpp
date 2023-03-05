#include<bits/stdc++.h>

using namespace std;

void Exit(string error) {
    cout << error << '\n';
    cout << "Usage:\n";
    cout << "\"./program filename.bin\" to convert from bin to sstable\n";
    cout << "\"./program filename.sstable filename.index\" to convert from sstable to bin\n";
    exit(1);
}

bool checkFormat(string filename, string format) {
    // because filename must be name.format, so filename.size() >= 1 (min name size) + 1 ('.') + format.size()
    if (filename.size() < format.size() + 2) return false;
    return (filename.substr(filename.size() - format.size(), format.size()) == format) && filename[filename.size() - format.size() - 1] == '.';
}

string getName(string filename) {
    while (filename.back() != '.') filename.pop_back();
    filename.pop_back();
    return filename;
}

uint8_t getUint8_tFromBuffer(vector<uint8_t>& buffer, int l) {
    return buffer[l];
}

vector<bool> getVectorBoolFromBuffer(vector<uint8_t>& buffer, int l, int r) {
    vector<bool> result;
    while (l < r) result.push_back(getUint8_tFromBuffer(buffer, l++) == 1);
    return result;
}

string convertVectorBoolToString(vector<bool> data) {
    string result;
    for (bool x : data) {
        result += x ? '1' : '0';
    }
    return result;
}

string getStringFromBuffer(vector<uint8_t>& buffer, int l, int r) {
    string result;
    while (l < r && buffer[l]) {
        result += buffer[l++];
    }
    return result;
}

void parseBin(string filename) {
    auto input = fopen((filename + ".bin").c_str(), "r");
    if (!input) {
        Exit("can't open " + filename + ".bin file for reading");
    }

    auto output_sstable = fopen((filename + ".sstable").c_str(), "w");
    if (!output_sstable) {
        Exit("can't open " + filename + ".sstable file for writing");
    }

    auto output_index = fopen((filename + ".index").c_str(), "w");
    if (!output_index) {
        Exit("can't open " + filename + ".index file for writing");
    }

    cout << "Reading binary data from " << filename << ".bin" << endl;

    vector<uint8_t> buffer(128);
    size_t read = 0;
    size_t student_id = 0;

    map<string, string> sstable;

    while ((read = fread(buffer.data(), sizeof(uint8_t), buffer.size(), input)) == buffer.size()) {
        cout << "student with id " << student_id << " read" << endl;
        string prefix(to_string(student_id) + "_");
        // name, size = 32, shift = 0
        sstable[prefix + "name"] = getStringFromBuffer(buffer, 0, 32);
        // login, size = 16, shift = 32
        sstable[prefix + "login"] = getStringFromBuffer(buffer, 32, 32 + 16);
        // group, size = 8, shift = 48
        sstable[prefix + "group"] = getStringFromBuffer(buffer, 48, 48 + 8);
        // practice, size = 8, shift = 56
        sstable[prefix + "practice"] = convertVectorBoolToString(getVectorBoolFromBuffer(buffer, 56, 56 + 8));
        // repo, size = 59, shift = 64
        sstable[prefix + "repo"] = getStringFromBuffer(buffer, 64, 64 + 59);
        // mark, size = 1, shift = 123
        sstable[prefix + "mark"] = to_string(getUint8_tFromBuffer(buffer, 123));
        // mark_float, size = 4, shift = 124
        sstable[prefix + "mark_float"] = to_string(*reinterpret_cast<uint32_t*>(buffer.data() + 124));
        cout << "student with id " << student_id << " parsed" << endl;
        student_id++;
    }
    if (read != 0) {
        cout << "Error while reading student with id " << student_id << ", it's not 128 bytes" << endl;
    }

    size_t shift = 0;
    for (auto& [key, value] : sstable) {
        fprintf(output_sstable, "%s%s", key.c_str(), value.c_str());
        fprintf(output_index, "%s %zu\n", key.c_str(), shift);
        shift += key.size() + value.size(); 
    }

    cout << "Conver binary file to sstable done\n";
    cout << "sstable saved to: " << filename + ".sstable\n";
    cout << "index saved to: " << filename + ".index";
    cout << endl;
}

void parseSstable(string sstableName, string indexName) {
    auto input_sstable = fopen((sstableName + ".sstable").c_str(), "r");
    if (!input_sstable) {
        Exit("can't open " + sstableName + ".sstable for reading");
    }
    
    auto input_index = fopen((indexName + ".index").c_str(), "r");
    if (!input_index) {
        Exit("can't open " + indexName + ".index for reading");
    }

    auto output = fopen((sstableName + ".bin").c_str(), "w");
    if (!output) {
        Exit("can't open " + sstableName + ".bin for writing");
    }

    cout << "parsing sstable started" << endl;
    vector<uint8_t> sstable;
    uint8_t x;
    while (fread(&x, sizeof(uint8_t), 1, input_sstable)) {
        sstable.push_back(x);
    }
    cout << "parsing sstable finished" << endl;

    cout << "parsing index started" << endl;
    string curr_name = "";
    size_t curr_shift = 0;
    bool shift_now = false;
    map<string, size_t> shifts;
    while (fread(&x, sizeof(uint8_t), 1, input_index)) {
        if (x == ' ') {
            shift_now = true;
            continue;
        }
        if (x == '\n') {
            shifts[curr_name] = curr_shift;
            shift_now = false;
            curr_name.clear();
            curr_shift = 0;
            continue;
        }
        if (shift_now) {
            curr_shift *= 10;
            curr_shift += x - '0';
        } else {
            curr_name += char(x);
        }
    }
    cout << "parsing index finished" << endl;

    cout << sstable.size() << endl;
    size_t id = 0;
    while (true) {
        string prefix(to_string(id) + "_");
        if (shifts.count(prefix + "name") == 0) break; // no student with such index
        int l, r;
        // name
        auto it = shifts.find(prefix + "name");
        l = it->second + it->first.size();
        it++;
        r = (it == shifts.end() ? sstable.size() : it->second) - 1;
        for (int i = 0; i < 32; ++i) {
            fprintf(output, "%c", l + i <= r ? sstable[l + i] : '\0');
        }
        // login
        it = shifts.find(prefix + "login");
        l = it->second + it->first.size();
        it++;
        r = (it == shifts.end() ? sstable.size() : it->second) - 1;
        for (int i = 0; i < 16; ++i) {
            fprintf(output, "%c", l + i <= r ? sstable[l + i] : '\0');
        }
        // group
        it = shifts.find(prefix + "group");
        cout << it->first << " " << it->second << endl;
        l = it->second + it->first.size();
        it++;
        r = (it == shifts.end() ? sstable.size() : it->second) - 1;
        for (int i = 0; i < 8; ++i) {
            fprintf(output, "%c", l + i <= r ? sstable[l + i] : '\0');
        }
        // practice
        it = shifts.find(prefix + "practice");
        l = it->second + it->first.size();
        it++;
        r = (it == shifts.end() ? sstable.size() : it->second) - 1;
        for (int i = 0; i < 8; ++i) {
            fprintf(output, "%c", sstable[l + i] - '0');
        }
        // repo
        it = shifts.find(prefix + "repo");
        l = it->second + it->first.size();
        it++;
        r = (it == shifts.end() ? sstable.size() : it->second) - 1;
        for (int i = 0; i < 59; ++i) {
            fprintf(output, "%c", l + i <= r ? sstable[l + i] : '\0');
        }
        // mark
        it = shifts.find(prefix + "mark");
        l = it->second + it->first.size();
        it++;
        r = (it == shifts.end() ? sstable.size() : it->second) - 1;
        uint8_t mark = 0;
        for (; l <= r; ++l) {
            mark *= 10;
            mark += sstable[l] - '0';
        }
        fprintf(output, "%c", mark);
        // mark_float
        it = shifts.find(prefix + "mark_float");
        l = it->second + it->first.size();
        it++;
        r = (it == shifts.end() ? sstable.size() : it->second) - 1;
        uint64_t mark_float = 0;
        for (; l <= r; ++l) {
            mark_float *= 10;
            mark_float += sstable[l] - '0';
        }
        uint8_t *mark_float_str = reinterpret_cast<uint8_t*>(&mark_float);
        for (int i = 0; i < 4; ++i) {
            fprintf(output, "%c", mark_float_str[i]);
        }
        id++;
    }

    cout << "convert students from sstable to bin finished\n";
    cout << "bin file saved to " << sstableName + ".bin";
    cout << endl;
}


int main(int argc, char *argv[]) {
    if (argc == 1) {
        Exit("File not given");
    } else if (argc == 2) {
        if (!checkFormat(argv[1], "bin")) Exit("Given file is not a bin file");
        parseBin(getName(argv[1]));
    } else if (argc == 3) {
        if (!checkFormat(argv[1], "sstable")) Exit("First given file is not a sstable file");
        if (!checkFormat(argv[2], "index")) Exit("Second given file is not a index file");
        parseSstable(getName(argv[1]), getName(argv[2]));
    }
}