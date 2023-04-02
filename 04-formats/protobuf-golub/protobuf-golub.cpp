#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "protobuf-golub.pb.h"

using namespace std;

struct Student {
    // имя может быть и короче 32 байт, тогда в хвосте 000
    // имя - валидный UTF-8
    char name[32] = {0};

    // ASCII [\w]+
    char login[16] = {0};
    char group[8] = {0};

    // 0/1, фактически bool
    uint8_t practice[8] = {0};

    struct {
        // URL
        char repo[59] = {0};
        uint8_t mark = 0;
    } project;

    // 32 bit IEEE 754 float
    float mark = 0; 
};

bool ends_with(const string& str, const string& end) {
    if (end.size() > str.size()) {
        return false;
    }
    return equal(end.rbegin(), end.rend(), str.rbegin());
}

void buildProtoStudent(format04::Student* res, const Student& student) {
    res->set_name(student.name);
    res->set_login(student.login);
    res->set_group(student.group);

    uint32_t practice = 0;
    for (uint32_t i = 0; i < 8; i++) {
        if (student.practice[i]) {
            practice |= (1u << i);
        }
    }
    res->set_practice(practice);

    res->set_project_repo(student.project.repo);
    res->set_project_mark(student.project.mark);
    res->set_mark(student.mark);
}

Student parseProtoStudent(const format04::Student& student) {
    Student res;
    student.name().copy(res.name, sizeof(res.name));
    student.login().copy(res.login, sizeof(res.login));
    student.group().copy(res.group, sizeof(res.group));
    for (uint32_t i = 0; i < 8; i++) {
        if (student.practice() & (1u << i)) {
            res.practice[i] = 1;
        }
    }
    student.project_repo().copy(res.project.repo, sizeof(res.project.repo) - 1);
    res.project.mark = student.project_mark();
    res.mark = student.mark();

    return res;
}

void SerializeToProtobuf(const string& path) {
    cout << "Reading binary student data from " << path << endl;

    auto input = fopen(path.c_str(), "r");
    Student data;
    vector<Student> students;

    while(fread(&data, sizeof(Student), 1, input)) {
        students.push_back(std::move(data));
    }
    fclose(input);
    cout << students.size() << " students read..." << endl;

    format04::Students result;
    for (const auto& student : students) {
        auto* protoStudent = result.add_student();
        buildProtoStudent(protoStudent, student);
    }

    const string output_path = path.substr(0, path.size() - 4) + ".protobuf";

    cout << "written to " << output_path << "..." << endl;
    ofstream output(output_path, ios::trunc | ios::out | ios::binary);
    result.SerializePartialToOstream(&output);
    output.close();
}

void SerializeToBin(const string& path) {
    cout << "Reading protobuf student data from " << path << endl;

    ifstream input(path, ios::binary | ios::in);
    format04::Students protoStudents;
    protoStudents.ParseFromIstream(&input);
    input.close();

    std::vector<Student> binStudents;
    for (auto& student : protoStudents.student()) {
        binStudents.push_back(parseProtoStudent(student));
    }

    cout << binStudents.size() << " students read..." << endl;

    const string output_path = path.substr(0, path.size() - 9) + ".bin";
    cout << "written to " << output_path << "..." << endl;
    ofstream output(output_path, ios::trunc | ios::out | ios::binary);

    for (const auto student : binStudents) {
        output.write((char*)&student, sizeof(Student));
    }
    output.close();
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Specify the path to the input file to convert as arg" << endl;
        return 1;
    }
    const string path = argv[1];
    if (ends_with(path, ".bin")) {
        SerializeToProtobuf(path);
    } else if (ends_with(path, ".protobuf")) {
        SerializeToBin(path);
    } else {
        cout << "Invalid file name" << endl;
        return 1;
    }
}