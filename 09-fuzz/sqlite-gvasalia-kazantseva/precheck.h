#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <cstdint>
#include <sqlite3.h>
#include <cmath>
#include <sstream>
#include <string>
#include <iomanip>
#include <limits>


using namespace std;

class MyException : public std::exception {
    private:
        std::string message;

    public:
        MyException(const std::string& msg) : message(msg) {}

        virtual const char* what() const throw ()
        {
            return message.c_str();
        }
};


struct StudentCheck {
    // имя может быть и короче 32 байт, тогда в хвосте 000
    // имя - валидный UTF-8
    char name[32];
    // ASCII [\w]+
    char login[16];
    char group[8];
    // 0/1, фактически bool
    uint8_t practice[8];
    struct {
        // URL
        char repo[59];
        uint8_t mark;
    } project;
    // 32 bit IEEE 754 float
    float mark;
};


void fail(const char *where) {
    throw MyException(std::string(where));
}


void check_str(const char *str, int len, const char *name) {
    int i = 0;
    while (i < len && str[i] != 0)
        i++;
    while (i < len && str[i] == 0)
        i++;
    if (i != len)
        fail(name);
}


bool checkStudent(StudentCheck& student) {
    try {
        check_str(student.name, 32, "name");
        check_str(student.login, 16, "login");
        check_str(student.group, 8, "group");
        for (int p = 0; p < 8; p++) {
        if (student.practice[p] != 0 && student.practice[p] != 1) {
            fail("practice");
        }
        }
        check_str(student.project.repo, 59, "repo");
        if (student.project.mark < 0 || student.project.mark > 10) {
            fail("project mark");
        }

        if (student.mark < 0 || student.mark > 10 || isnan(student.mark)) {

            fail("mark");
        }

    } catch (MyException& ex) {
        fprintf(stderr, "Malformed input at %s\n", ex.what());
        return false;

    }
    return true;
}


bool CheckData(const uint8_t * data, size_t size) {
    const size_t RECSIZE = sizeof(StudentCheck);
    size_t i = 0;

    if (RECSIZE > size) {
        return false;
    }

    if (size % RECSIZE != 0) {
        return false;
    }

    while (i + RECSIZE <= size) {
        struct StudentCheck student;
        memcpy(&student, data + i, RECSIZE);
        i += RECSIZE;
        bool isStudentDataCorrect = checkStudent(student);
        if (!isStudentDataCorrect) {
            return false;
        }
    }

    return true;
}