#include "tinyxml2.h"
#include "xml-nasonkov.h"

#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <cctype>
#include <cstdlib>

#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <limits>
#include <fstream>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <cstdint>
#include <cmath>
#include <sstream>
#include <string>
#include <iomanip>

using namespace std;

using namespace tinyxml2;

int RECSIZE = sizeof(struct Student);

bool check_str(const char *str, int len, const char *name) {
    int i = 0;
    while (i < len && str[i] != 0)
        i++;
    //std::cout << i << std::endl;
    if (i == 0) {
        return false;
    }
    while (i < len && str[i] == 0)
        i++;
    return i == len;
}

bool check_double(float a) {
    if ((float)((int)(a * 10)) / 10.0 == a) {
        return false;
    }
    return true;
}

bool check_special_symbols(char* str, int sz) {
    for (int i = 0; i < sz; i++) {
        if (str[i] == 10 || str[i] == 9 || str[i] == 13 || str[i] == 32) {
            return true;
        }
    }
    return false;
}

bool validate_input(const char *file_name) {
    struct Student student;
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        return false;
    }
    ssize_t rd = 0;
    while (RECSIZE == (rd = read(fd, &student, RECSIZE))) {
        if (!check_str(student.name, 32, "name") || check_special_symbols(student.name, 32)) {
             close(fd);
            return false;
        }
        if (!check_str(student.login, 16, "login") || check_special_symbols(student.name, 16)) {
             close(fd);
             return false;
        }
        if (!check_str(student.group, 8, "group") || check_special_symbols(student.name, 8)) {
             close(fd);
             return false;
        }
        for (int p = 0; p < 8; p++) {
        if (student.practice[p] != 0 && student.practice[p] != 1) {
             close(fd);
             return false;
        }
        }
        if (!check_str(student.project.repo, 59, "repo") || check_special_symbols(student.name, 59)) {
             close(fd);
            return false;
        }
        if (student.mark < 0 || student.mark > 10 || isnan(student.mark) || check_double(student.mark)) {
             close(fd);
            return false;
        }

        // if (student.project.mark < 0 || student.project.mark > 10)
        // goto cleanup;
        if (student.mark < 0 || student.mark > 10) {
             close(fd);
         return false;
        }
    }
    cleanup:
    close(fd);
    return rd == 0;
}

char* make_c_string(char* string, int len) {
    char* p = (char*)malloc(len+1);
    p[len] = 0;
    memcpy(p, string, len);
    return p;
}

void pretty_string_print(char* str, int size) {
    for (int i = 0; i < size; i++) {
        std::cout << (int)((uint8_t)str[i]) << " ";
    }
    std::cout << std::endl;
}

void pretty_print(Student student) {
    std::cout << "name\n";
    pretty_string_print(student.name, 32);
    std::cout << "login\n";
    pretty_string_print(student.login, 16);
    std::cout << "group\n";
    pretty_string_print(student.group, 8);
    std::cout << "practice\n";
    pretty_string_print((char*)student.practice, 8);
    std::cout << "repo\n";
    pretty_string_print(student.project.repo, 59);
    std::cout << "project mark\n";
    std::cout << (int)student.project.mark << "\n";
     std::cout << "mark\n";
    pretty_string_print((char*)(&student.mark), 4);
}

bool fromBinToXML(const char* filename_) {
    char* filename = (char*)malloc(strlen(filename_) + 1);
    memcpy(filename, filename_, strlen(filename_) + 1);
    std::ifstream input(filename, std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "Error while opening the file!\n";
        exit(1);
    }

    if (!validate_input(filename)) {
        free(filename);
        return false;
    }

    //std::cout << "Reading binary data from " << filename << "\n";
    int cnt_students = 0;

    XMLDocument xmlDoc;
    XMLElement* root_elem = xmlDoc.NewElement("students");

    while (!input.eof()) {
        Student student;
        if (!input.read(reinterpret_cast<char*>(&student), sizeof student)) {
            break;
        }
       // pretty_print(student);
        ++cnt_students;

        XMLElement* cur_student = xmlDoc.NewElement("student");

        XMLElement* cur_student_name = xmlDoc.NewElement("name");
        auto str1 = make_c_string(student.name, 32);
        cur_student_name->SetText(str1);
        //free(str1);

        cur_student->InsertEndChild(cur_student_name);

        XMLElement* cur_student_login = xmlDoc.NewElement("login");
        // char login[17];
        // std::memcpy(login, student.login, 16);
        // login[16] = '\0';
        /*size_t ind = 16;
        while (!std::isalpha(login[ind]) && !std::isdigit(login[ind]) && login[ind] != '_') {
            login[ind] = 0;
            --ind;
        }*/
        auto str2 = make_c_string(student.login, 16);
        cur_student_login->SetText(str2);
        //free(str2);
        cur_student->InsertEndChild(cur_student_login);

        XMLElement* cur_student_group = xmlDoc.NewElement("group");
        //char group[9];
        // std::memcpy(group, student.group, 8);
        // group[8] = '\0';
        /*ind = 8;
        while (!std::isalpha(group[ind]) && !std::isdigit(group[ind]) && group[ind] != '_') {
            group[ind] = '\0';
            --ind;
        }*/
        auto str3 = make_c_string(student.group, 8);
        cur_student_group->SetText(str3);
        //free(str3);

        cur_student->InsertEndChild(cur_student_group);

        XMLElement* cur_student_practice = xmlDoc.NewElement("practice");
        std::string practice = "";
        for (int i = 0; i < 8; ++i) {
            practice += (student.practice[i] == 0 ? '0' : '1');
        }
        cur_student_practice->SetText(practice.c_str());
        cur_student->InsertEndChild(cur_student_practice);

        XMLElement* cur_student_project = xmlDoc.NewElement("project");
        XMLElement* cur_student_project_repo = xmlDoc.NewElement("repo");
        //std::cout << "len " << strlen(student.project.repo) << std::endl;
        auto str4 = make_c_string(student.project.repo, 59);
        cur_student_project_repo->SetText(str4);
        //free(str4);
        cur_student_project->InsertEndChild(cur_student_project_repo);

        XMLElement* cur_student_project_mark = xmlDoc.NewElement("mark");
        cur_student_project_mark->SetText(student.project.mark);
        cur_student_project->InsertEndChild(cur_student_project_mark);

        cur_student->InsertEndChild(cur_student_project);

        XMLElement* cur_student_mark = xmlDoc.NewElement("mark");
        //std::cout << "double " << (double)student.mark << std::endl;

        //auto str5 = make_c_string((char*)&student.mark, 4);
        cur_student_mark->SetText(student.mark);
        cur_student->InsertEndChild(cur_student_mark);

        root_elem->InsertEndChild(cur_student);

        free(str1);
        free(str2);
        free(str3);
        free(str4);
    }
    input.close();
    //std::cout << "Read info about " << cnt_students << " student(s)\n";

    xmlDoc.InsertFirstChild(root_elem);
    size_t filename_len = std::strlen(filename);
    filename[filename_len - 3] = 'x';
    filename[filename_len - 2] = 'm';
    filename[filename_len - 1] = 'l';
    XMLError saving_result = xmlDoc.SaveFile(filename);

    if (saving_result == XMLError::XML_SUCCESS) {
        //std::cout << "XML successfully generated and saved as " << filename << "\n";
        // free(str1);
        // free(str2);
        // free(str3);
        // free(str4);

        free(filename);
        return true;
    }
    free(filename);
    //std::cerr << "Error " << saving_result << " while saving result XML as " << filename << "\n";
    exit(1);
}

void fromXMLToBin(const char* filename_) {
    char* filename = (char*)malloc(strlen(filename_) + 1);
    memcpy(filename, filename_, strlen(filename_) + 1);
    XMLDocument xmlDoc;
    XMLError loading_result = xmlDoc.LoadFile(filename);
    if (loading_result != XMLError::XML_SUCCESS) {
        std::cerr << "Error " << loading_result << " while loading XML from provided file!\n";
        free(filename);
        exit(1);
    }
    //std::cout << "Reading xml data from " << filename << "\n";

    XMLNode* root_elem = xmlDoc.FirstChild();
    int cnt_students = 0;

    size_t filename_len = std::strlen(filename);
    filename[filename_len - 3] = 'b';
    filename[filename_len - 2] = 'i';
    filename[filename_len - 1] = 'n';
    std::ofstream output(filename, std::ios::binary);
    XMLElement* cur_student = root_elem->FirstChildElement("student");
    while (cur_student) {
        ++cnt_students;
        Student student;
        const char* text_ptr;
        size_t ind;
        // name
        text_ptr = cur_student->FirstChildElement("name")->GetText();
        ind = 0;
        while (ind < 32 && text_ptr[ind] != 0) {
            student.name[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 32) {
            student.name[ind] = 0;
            ++ind;
        }

        // login
        text_ptr = cur_student->FirstChildElement("login")->GetText();
        ind = 0;
        while (ind < 16 && text_ptr[ind] != 0) {
            student.login[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 16) {
            student.login[ind] = 0;
            ++ind;
        }

        // group
        text_ptr = cur_student->FirstChildElement("group")->GetText();
        ind = 0;
        while (ind < 8 && text_ptr[ind] != 0) {
            student.group[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 8) {
            student.group[ind] = 0;
            ++ind;
        }

        // practice
        text_ptr = cur_student->FirstChildElement("practice")->GetText();
        ind = 0;
        while (ind < 8) {
            student.practice[ind] = static_cast<uint8_t>(text_ptr[ind] - '0');
            ++ind;
        }

        // project
        XMLElement* cur_student_project = cur_student->FirstChildElement("project");
        // repo
        text_ptr = cur_student_project->FirstChildElement("repo")->GetText();
        ind = 0;
        while (ind < 59 && text_ptr[ind] != 0) {
            student.project.repo[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 59) {
            student.project.repo[ind] = 0;
            ++ind;
        }

        // mark for project
        char* tmp;
        long project_mark = std::strtol(cur_student_project->FirstChildElement("mark")->GetText(), &tmp, 10);
        student.project.mark = static_cast<uint8_t>(project_mark);

        // mark
        student.mark = std::strtof(cur_student->FirstChildElement("mark")->GetText(), &tmp);
        // double a;
        // XMLUtil::ToDouble(cur_student->FirstChildElement("mark")->GetText(), &a);
        // student.mark = a;

        output.write(reinterpret_cast<char*>(&student), sizeof student);

        cur_student = cur_student->NextSiblingElement("student");
    }
    free(filename);
    output.close();
    //std::cout << "Read info about " << cnt_students << " student(s)\n";
    //std::cout << "XML successfully parsed to binary format and saved as " << filename << "\n";
}
