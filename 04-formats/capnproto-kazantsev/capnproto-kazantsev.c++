#include "kj/io.h"
#include "student.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>

struct Student {
    // имя может быть и короче 32 байт, тогда в хвосте 000
    // имя - валидный UTF-8
    char    name[32];
    // ASCII [\w]+
    char    login[16];
    char    group[8];
    // 0/1, фактически bool
    uint8_t practice[8];
    struct {
        // URL
        char    repo[59];
        uint8_t mark;
    } project;
    // 32 bit IEEE 754 float
    float   mark; 
};

void dumpCapnproto(int fd, Student& bin_student) {
    ::capnp::MallocMessageBuilder message;

    auto student = message.initRoot<capn_student::Student>();
    student.setName(bin_student.name);
    student.setLogin(bin_student.login);
    student.setGroup(bin_student.group);

    auto practice = student.initPractice(8);
    for (size_t i = 0; i < 8; ++i) {
        practice.set(i, bin_student.practice[i]);
    }

    auto project = student.initProject();
    project.setRepo(bin_student.project.repo);
    project.setMark(bin_student.project.mark);
    
    student.setMark(bin_student.mark);
    
    ::capnp::writePackedMessageToFd(fd, message);
}

std::pair<bool, Student> readCapnProto(int fd) {
    try {
        ::capnp::PackedFdMessageReader message(fd);

        capn_student::Student::Reader capn_student = message.getRoot<capn_student::Student>();
        Student student;
        strncpy(student.name, capn_student.getName().cStr(), sizeof(student.name));
        strncpy(student.login, capn_student.getLogin().cStr(), sizeof(student.login));
        strncpy(student.group, capn_student.getGroup().cStr(), sizeof(student.group));

        auto practice = capn_student.getPractice();
        for (size_t i = 0; i < 8; ++i) {
            student.practice[i] = practice[i];
        }

        auto project = capn_student.getProject();
        strncpy(student.project.repo, project.getRepo().cStr(), sizeof(student.project.repo));
        student.project.mark = project.getMark();

        student.mark = capn_student.getMark();

        return {true, student};
    } catch (const kj::Exception &e) {
        return {false, {}};
    }
}

void dumpStudent(std::ofstream& out, Student& student) {
    out.write((char*)&student, sizeof(student));
}

void FromCapnprotoToStudent(std::string& name) {
    std::cout << "Open file " << name << ".capnproto for reading" << std::endl;
    int fd = open((name + ".capnproto").c_str(), O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("Cannot open file " + name + ".capnproto for reading");
    }

    std::cout << "Open file " << name << ".bin for dumping" << std::endl;
    std::ofstream out(name + ".bin", std::ios::binary | std::ios::trunc);

    while (true) {
        std::cout << "Read.." << std::endl;
        auto [success, student] = readCapnProto(fd);
        if (!success) {
            break;
        }
        dumpStudent(out, student);
    }

    out.close();
    close(fd);
}

void FromStudentToCapnproto(std::string& name) {
    std::cout << "Open file " << name << ".bin for reading" << std::endl;
    auto in = fopen((name + ".bin").c_str(), "r");

    std::cout << "Open file " << name << ".capnproto for dumping" << std::endl;
    int fd = open((name + ".capnproto").c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if (fd == -1) {
        fclose(in);
        throw std::runtime_error("Cannot open file " + name + ".capnproto for dumping");
    }

    Student student;
    while(fread(&student, sizeof(Student), 1, in)) {
        dumpCapnproto(fd, student);
    }
    fclose(in);
    close(fd);
}

std::pair<std::string, std::string> parseFileName(const std::string& filename) {
    if (filename.empty()) {
        throw std::runtime_error("Empty filename");
    }

    size_t pos = filename.find_last_of(".");
    std::string name = filename.substr(0, pos);
    std::string extension = filename.substr(pos);

    return {name, extension};
}  

int main(int argc, char *argv[]) {
    if (argc != 2 || !argv[1]) {
        throw std::runtime_error("Wrong arguments count");
    }
    
    std::string filename = argv[1];
    auto [name, extension] = parseFileName(filename);

    if (extension == ".capnproto") {
        FromCapnprotoToStudent(name);
    } else if (extension == ".bin") {
        FromStudentToCapnproto(name);
    } else {
        throw std::runtime_error("Unknown file extension");
    }

    return 0;
}
