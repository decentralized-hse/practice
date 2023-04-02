#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include <flatbuffers/flatbuffers.h>
#include "StudentFormat/schema_generated.h"

using namespace std;

struct __attribute__((aligned(4))) Student {
    // имя может быть и короче 32 байт, тогда в хвосте 000
    // имя - валидный UTF-8
    char    name[32];

    // ASCII [\w]+
    char    login[16];
    char    group[8];

    // 0/1, фактически bool
    static const size_t practiceSize = 8;
    uint8_t practice[practiceSize];

    struct {
        // URL
        char    repo[59];
        uint8_t mark;
    } project;

    // 32 bit IEEE 754 float
    float   mark;
};

pair<string, string> parseInputFilename(const string& inputName) {
    auto last_separator = inputName.find_last_of(".");
    return {inputName.substr(0, last_separator), inputName.substr(last_separator + 1)};
}

void writeStructAsFlatbuffer(const Student& student, fstream& out, std::vector<StudentFormat::Student>& students) {

    std::array<uint8_t, 59> projectRepo;
    std::move(std::begin(student.project.repo), std::end(student.project.repo), projectRepo.begin());
    std::array<uint8_t, 32> name;
    std::move(std::begin(student.name), std::end(student.name), name.begin());
    std::array<uint8_t, 16> login;
    std::move(std::begin(student.login), std::end(student.login), login.begin());
    std::array<uint8_t, 8> group;
    std::move(std::begin(student.group), std::end(student.group), group.begin());
    std::array<uint8_t, 8> practice;
    std::move(std::begin(student.practice), std::end(student.practice), practice.begin());

    StudentFormat::StudentProject currentProject(projectRepo, student.project.mark);
    students.push_back(StudentFormat::Student(name, login, group, practice, currentProject, student.mark));
}

void convertToFlatbuffer(const string& inputName) {
    //C style files for C style structs to be sage
    auto inPointer = unique_ptr<FILE, function<void(FILE*)>>(
        fopen((inputName + ".bin").c_str(), "r"),
        [](FILE* fp) {fclose(fp);});
    fstream file;
    file.open(inputName + ".flat", std::ios::out | std::ios::binary);

    Student student;
    size_t id = 0;
    std::vector<StudentFormat::Student> students;
    while(fread(&student, sizeof(Student), 1, inPointer.get()) == 1) {
        writeStructAsFlatbuffer(student, file, students);
        ++id;
    }
    flatbuffers::FlatBufferBuilder builder;
    auto allStudents = StudentFormat::CreateStudentMessageDirect(builder, &students);
    builder.Finish(allStudents);
    file.write(reinterpret_cast<char*>(builder.GetBufferPointer()), builder.GetSize());
    file.close();
    cout << "Read " << id << " students, output file: " << inputName << ".flat" << endl;
}

template<typename V, int T>
void loadData(V* to, const flatbuffers::Array<uint8_t, T>* from) {
    memcpy((char*)to, from, T);
}

void writeStudentToOutput(const StudentFormat::Student* student, FILE* outPointer) {
    Student studentToOutput;
    loadData<char, 32>(studentToOutput.name, student->name());
    loadData<char, 16>(studentToOutput.login, student->login());
    loadData<char, 8>(studentToOutput.group, student->group());
    loadData<uint8_t, 8>(studentToOutput.practice, student->practice());
    loadData<char, 59>(studentToOutput.project.repo, student->project().repo());
    studentToOutput.project.mark = student->project().mark();
    studentToOutput.mark = student->mark();
    fwrite(&studentToOutput, sizeof(studentToOutput), 1, outPointer);
}

void convertToBin(const string& inputName) {
    auto outPointer = unique_ptr<FILE, function<void(FILE*)>>(
        fopen((inputName + ".bin").c_str(), "w+"),
        [](FILE* fp) {fclose(fp);});
    fstream file;
    file.open(inputName + ".flat", std::ios::binary | std::ios::in);
    file.seekg(0,std::ios::end);
    int length = file.tellg();
    file.seekg(0,std::ios::beg);
    unique_ptr<char> data(new char[length]);
    file.read(data.get(), length);
    file.close();

    auto students = StudentFormat::GetStudentMessage(data.get());
    for (const StudentFormat::Student* student : (*students->studentimpl())) {
        writeStudentToOutput(student, outPointer.get());
    }
    cout << "Write " << (*students->studentimpl()).size() << " students, output file: " << inputName << ".bin" << endl;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: file to process" << endl;
        return 1;
    }
    auto [filename, format] = parseInputFilename(argv[1]);
    if (format == "bin") {
        convertToFlatbuffer(filename);
    } else if (format == "flat") {
        convertToBin(filename);
    } else {
        cout << "File format should be either flat or bin. Got " << format << endl;
        return 1;
    }
    return 0;
}
