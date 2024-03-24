#include <capnp/serialize.h>
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


void getCorrectString(char* a, char* b, size_t size) {
    for (int i = 0; i < size; ++i) {
        a[i] = b[i];
    }
    a[size] = '\0';
}

void dumpCapnproto(int fd, Student& bin_student) {
    std::cout << "damp capnproto\n";
    ::capnp::MallocMessageBuilder message;

    auto student = message.initRoot<capn_student::Student>();

    char name[33];
    getCorrectString(name, bin_student.name, 32);
    student.setName(name);
    // Если строка не заканчивалась на '\0',
    // то при вызове studen.set...(...) был проезд по памяти. 
    // Добавила конвертацию строк в корректные строки

    char login[17];
    getCorrectString(login, bin_student.login, 16);
    student.setLogin(login);

    char group[9];
    getCorrectString(group, bin_student.group, 8);
    student.setGroup(group);

    auto practice = student.initPractice(8);
    for (size_t i = 0; i < 8; ++i) {
        practice.set(i, bin_student.practice[i]);
    }

    auto project = student.initProject();

    char repo[60];
    getCorrectString(repo, bin_student.project.repo, 59);
    project.setRepo(repo);

    project.setMark(bin_student.project.mark);
    
    student.setMark(bin_student.mark);
    
    std::cout << "Write.." << std::endl;
    ::capnp::writeMessageToFd(fd, message);
}

std::pair<bool, Student> readCapnProto(int fd) {
    try {
        ::capnp::StreamFdMessageReader message(fd);

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
    std::cout << "Write student.." << std::endl;
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

    std::cout << "Open file " << name << ".capnproto for dumping" << std::endl;
    int fd = open((name + ".capnproto").c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if (fd == -1) {
        throw std::runtime_error("Cannot open file " + name + ".capnproto for dumping");
    }

    std::cout << "Open file " << name << ".bin for reading" << std::endl;
    std::ifstream in(name + ".bin", std::ios::binary);

    Student student;
    // while(fread(&student, sizeof(Student), 1, in)) {
    //     dumpCapnproto(fd, student);
    // }
    // Не было проверки, что чтение успешно. При частичном прочтении значение не определено. Я решила поменять на in.read
    while (in.read((char*) &student, sizeof(Student))) {
        if (in.gcount() != sizeof(Student)) {
            in.close();
            return;
        }
        dumpCapnproto(fd, student);
    }
    in.close();
    close(fd);
}
