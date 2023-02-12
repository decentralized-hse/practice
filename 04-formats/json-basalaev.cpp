#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

struct Student {
    char    name[32];
    char    login[16];
    char    group[8];
    static const size_t practiceSize = 8;
    uint8_t practice[practiceSize];
    struct {
        char    repo[59];
        uint8_t mark;
    } project;
    float   mark;
};

struct TaskAux {
    enum class Type {
        JsonToBinary = 1,
        BinaryToJson = 2,
        Unknown = 3,
    };

    const std::string inputFilename;
    const std::string outputFilename;
    Type type;
};

std::vector<Student> readBinaryStudents(const TaskAux& taskAux) {
    auto in = fopen(taskAux.inputFilename.c_str(), "r");
    std::cout << "Reading binary students data from "
              << taskAux.inputFilename << std::endl;

    Student student;
    std::vector<Student> result;

    while(fread(&student, sizeof(Student), 1, in)) {
        result.emplace_back(student);
    }

    std::cout << result.size() << " students read..." << std::endl;

    return result;
}

void writeStudentsIntoJson(const std::vector<Student>& students, const TaskAux& taskAux) {
    std::ofstream out(taskAux.outputFilename);

    auto wrapStr = [](const std::string& field, const std::string& str, std::ofstream& out) {
        out << "\"" << field << "\": " << "\"" << str << "\",\n";
    };

    out << "[\n";
    for (auto i = 0; i < students.size(); ++i) {
        if (i > 0) {
            out << ",\n";
        }
        out << "{\n";

        // dump strs
        wrapStr("name", students[i].name, out);
        wrapStr("login", students[i].login, out);
        wrapStr("group", students[i].group, out);

        // dump practice
        out << "\"practice\": [";
        for (auto j = 0; j < students[i].practiceSize; j++) {
            if (j > 0) {
                out << ",";
            }
            out << std::to_string(students[i].practice[j]);
        }
        out << "],\n";

        // dump project
        out << "\"project\": \n{\n";
        wrapStr("repo", students[i].project.repo, out);
        out << "\"mark\": " << static_cast<uint32_t>(students[i].project.mark) << "\n";
        out << "},\n";

        // dump float
        out << "\"mark\": " << students[i].mark << "\n";

        out << "}";
    }
    out << "\n]\n";

    std::cout << "Written to " << taskAux.outputFilename;
}

void covertBinaryToJson(const TaskAux& taskAux) {
    auto students = readBinaryStudents(taskAux);
    writeStudentsIntoJson(students, taskAux);
}

void clearString(std::string& str) {
    const std::set<char> permitedSymbols{'\"', ',', '[', ']'};
    while (std::isspace(str.back()) || permitedSymbols.count(str.back())) {
        str.pop_back();
    }

    std::reverse(str.begin(), str.end());

    while (std::isspace(str.back()) || permitedSymbols.count(str.back())) {
        str.pop_back();
    }

    std::reverse(str.begin(), str.end());
}

void parseString(std::string& str, char out[]) {
    clearString(str);
    memcpy(out, str.c_str(), str.size());
}

std::vector<Student> readJsonStudents(const TaskAux& taskAux) {
    std::cout << "Reading json students data from "
              << taskAux.inputFilename << std::endl;

    std::ifstream in(taskAux.inputFilename);

    std::vector<Student> result;
    std::string line;
    bool projectFlag = false;
    Student student{};
    int bracketBalance = 0;
    while (getline(in, line)) {
        if (line == "{") {
            bracketBalance++;
            continue;
        }

        if (line == "}," || line == "}") {
            bracketBalance--;
            if (!bracketBalance) {
                result.emplace_back(student);
                student = Student{};
            }
            continue;
        }

        auto delimeter = line.find_first_of(":");
        if (delimeter == std::string::npos) {
            continue;
        }

        std::string fieldName = line.substr(1, delimeter - 2);
        std::string fieldValue = line.substr(delimeter + 1);

        if (fieldName == "project") {
            projectFlag = true;
            continue;
        }

        if (fieldName == "name") {
            parseString(fieldValue, student.name);
        } else if (fieldName == "login") {
            parseString(fieldValue, student.login);
        } else if (fieldName == "group") {
            parseString(fieldValue, student.group);
        } else if (fieldName == "repo") {
            parseString(fieldValue, student.project.repo);
        } else if (fieldName == "mark") {
            clearString(fieldValue);
            if (projectFlag) {
                student.project.mark = std::stoul(fieldValue);
                projectFlag = false;
            } else {
                student.mark = std::stof(fieldValue);
            }
        } else if (fieldName == "practice") {
            clearString(fieldValue);
            size_t idx = 0;
            for (auto i = 0; i < fieldValue.size(); ++i) {
                if (fieldValue[i] == '0' || fieldValue[i] == '1') {
                    student.practice[idx++] = fieldValue[i] - '0';
                }
            }
        }
    }

    std::cout << result.size() << " students read..." << std::endl;
    return result;
}

void writeStudentsIntoBin(const std::vector<Student>& students, const TaskAux& taskAux) {
    auto out = fopen(taskAux.outputFilename.c_str(), "w");
    for (const auto& student : students) {
        fwrite(&student, sizeof(Student), 1, out);
    }

    std::cout << "Written to " << taskAux.outputFilename;
}

void convertJsonToBinary(const TaskAux& taskAux) {
    auto students = readJsonStudents(taskAux);
    writeStudentsIntoBin(students, taskAux);
}

TaskAux getTaskType(const std::string& filename) {
    auto idx = filename.find_last_of(".");
    auto name = filename.substr(0, idx);
    auto type = filename.substr(idx + 1);

    if (type == "bin") {
        return TaskAux{
            .inputFilename = filename,
            .outputFilename = name + ".json",
            .type = TaskAux::Type::BinaryToJson
        };
    } else if (type == "json") {
        return TaskAux{
            .inputFilename = filename,
            .outputFilename = name + ".bin",
            .type = TaskAux::Type::JsonToBinary
        };
    }

    return TaskAux{
        .inputFilename = {},
        .outputFilename = {},
        .type = TaskAux::Type::Unknown
    };
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Does not given path parameter" << std::endl;
        return 1;
    }

    auto taskAux = getTaskType(argv[1]);

    switch (taskAux.type) {
        case TaskAux::Type::BinaryToJson:
            covertBinaryToJson(taskAux);
            break;
        case TaskAux::Type::JsonToBinary:
            convertJsonToBinary(taskAux);
            break;
        case TaskAux::Type::Unknown:
            std::cout << "Invalid file format" << std::endl;
            return 1;
    }

    return 0;
}
