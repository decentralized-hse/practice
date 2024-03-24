#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "json.hpp"

using namespace nlohmann;

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
    //std::cout << "Reading binary students data from "
 //              << taskAux.inputFilename << std::endl;

    Student student;
    std::vector<Student> result;

    while(fread(&student, sizeof(Student), 1, in)) {
        result.emplace_back(student);
    }

    //std::cout << result.size() << " students read..." << std::endl;

    fclose(in);  // fix

    return result;
}

void writeStudentsIntoJson(const std::vector<Student>& students, const TaskAux& taskAux) {
    std::ofstream out(taskAux.outputFilename);

    auto wrapStr = [](const std::string& field, const std::string& str, std::ofstream& out) {
        out << "\"" << field << "\": " << "\"" << str << "\",\n";
    };

    json result;
    for (auto i = 0; i < students.size(); ++i) {
        json currentStudent;

        currentStudent["name"] = students[i].name;
        currentStudent["login"] = students[i].login;
        currentStudent["group"] = students[i].group;

        for (auto j = 0; j < students[i].practiceSize; j++) {
            currentStudent["practice"].push_back(std::to_string(students[i].practice[j]));
        }

        currentStudent["project"] = {{"repo", students[i].project.repo}, {"mark", static_cast<uint32_t>(students[i].project.mark)}};
        currentStudent["mark"] = students[i].mark;

        result.push_back(currentStudent);
    }

    out << result;

    //std::cout << "Written to " << taskAux.outputFilename;
}

void covertBinaryToJson(const TaskAux& taskAux) {
    auto students = readBinaryStudents(taskAux);
    writeStudentsIntoJson(students, taskAux);
}

void parseString(const std::string& str, char out[]) {
    memcpy(out, str.c_str(), str.size());
}

std::vector<Student> readJsonStudents(const TaskAux& taskAux) {
    //std::cout << "Reading json students data from "
    //            << taskAux.inputFilename << std::endl;

    std::ifstream in(taskAux.inputFilename);
    json j = json::parse(in);

    std::vector<Student> result;
    Student student{};

    for (const auto& jsonStudent : j) {
        parseString(jsonStudent["name"].get<std::string>(), student.name);
        parseString(jsonStudent["login"].get<std::string>(), student.login);
        parseString(jsonStudent["group"].get<std::string>(), student.group);
        parseString(jsonStudent["project"]["repo"].get<std::string>(), student.project.repo);
        student.project.mark = jsonStudent["project"]["mark"].get<uint8_t>();
        student.mark = jsonStudent["mark"].get<float>();
        for (auto i = 0; i < jsonStudent["practice"].size(); ++i) {
            if (jsonStudent["practice"][i].get<std::string>() == "0")
                student.practice[i] = 0;
            else
                student.practice[i] = 1;
        }
        result.push_back(student);
        student = Student{};
    }

    //std::cout << result.size() << " students read..." << std::endl;
    return result;
}

void writeStudentsIntoBin(const std::vector<Student>& students, const TaskAux& taskAux) {
    auto out = fopen(taskAux.outputFilename.c_str(), "w");
    for (const auto& student : students) {
        fwrite(&student, sizeof(Student), 1, out);
    }
    fclose(out);  // fix

    //std::cout << "Written to " << taskAux.outputFilename;
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
