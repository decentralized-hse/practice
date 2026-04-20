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

    std::vector<uint8_t> inputData;
    std::vector<uint8_t> outputData;
    Type type;
};

std::vector<Student> readBinaryStudents(const std::vector<uint8_t>& data) {
    size_t numStudents = data.size() / sizeof(Student);
    const Student* studentsPtr = reinterpret_cast<const Student*>(data.data());
    return std::vector<Student>(studentsPtr, studentsPtr + numStudents);
}

std::string writeStudentsIntoJson(const std::vector<Student>& students) {
    json result;
    for (const auto& student : students) {
        json currentStudent;
        currentStudent["name"] = student.name;
        currentStudent["login"] = student.login;
        currentStudent["group"] = student.group;

        for (auto j = 0; j < student.practiceSize; j++) {
            currentStudent["practice"].push_back(std::to_string(student.practice[j]));
        }

        currentStudent["project"] = {{"repo", student.project.repo}, {"mark", static_cast<uint32_t>(student.project.mark)}};
        currentStudent["mark"] = student.mark;

        result.push_back(currentStudent);
    }
    return result.dump();
}

std::string covertBinaryToJson(const TaskAux& taskAux) {
    auto students = readBinaryStudents(taskAux.inputData);
    return writeStudentsIntoJson(students);
}

std::vector<Student> readJsonStudents(const std::string& jsonString) {
    std::vector<Student> result;
    auto j = json::parse(jsonString);
    for (const auto& jsonStudent : j) {
        Student student;
        strncpy(student.name, jsonStudent["name"].get<std::string>().c_str(), sizeof(student.name));
        strncpy(student.login, jsonStudent["login"].get<std::string>().c_str(), sizeof(student.login));
        strncpy(student.group, jsonStudent["group"].get<std::string>().c_str(), sizeof(student.group));
        for (int i = 0; i < student.practiceSize; ++i) {
            student.practice[i] = std::stoi(jsonStudent["practice"][i].get<std::string>());
        }
        strncpy(student.project.repo, jsonStudent["project"]["repo"].get<std::string>().c_str(), sizeof(student.project.repo));
        student.project.mark = jsonStudent["project"]["mark"].get<uint8_t>();
        student.mark = jsonStudent["mark"].get<float>();
        result.push_back(student);
    }
    return result;
}

std::vector<uint8_t> writeStudentsIntoBin(const std::vector<Student>& students) {
    std::vector<uint8_t> result;
    for (const auto& student : students) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&student);
        result.insert(result.end(), ptr, ptr + sizeof(Student));
    }
    return result;
}

std::vector<uint8_t> convertJsonToBinary(const TaskAux& taskAux) {
    std::string jsonString(taskAux.inputData.begin(), taskAux.inputData.end());
    auto students = readJsonStudents(jsonString);
    return writeStudentsIntoBin(students);
}

TaskAux getTaskType(std::vector<uint8_t>* binData, std::string* jsonData, TaskAux::Type type) {
    if (type == TaskAux::Type::BinaryToJson) {
        return TaskAux{
            .inputData = *binData,
            .outputData = {},
            .type = TaskAux::Type::BinaryToJson
        };
    } else if (type == "json") {
        std::vector<uint8_t> vec(jsonData->begin(), jsonData->end());
        return TaskAux{
            .inputData = vec,
            .outputData = {},
            .type = TaskAux::Type::JsonToBinary
        };
    }

    return TaskAux{
        .inputData = {},
        .outputData = {},
        .type = TaskAux::Type::Unknown
    };
}
