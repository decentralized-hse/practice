#include "tinyxml2.h"
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <cctype>
#include <cstdlib>
using namespace tinyxml2;

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

char bin_extension[4] = "bin";
char xml_extension[4] = "xml";

void fromBinToXML(char* filename) {
    std::ifstream input(filename, std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "Error while opening the file!\n";
        exit(1);
    }
    std::cout << "Reading binary data from " << filename << "\n";
    int cnt_students = 0;

    XMLDocument xmlDoc;
    XMLElement* root_elem = xmlDoc.NewElement("students");

    while (!input.eof()) {
        Student student;
        if (!input.read(reinterpret_cast<char*>(&student), sizeof student)) {
            break;
        }
        ++cnt_students;

        XMLElement* cur_student = xmlDoc.NewElement("student");

        XMLElement* cur_student_name = xmlDoc.NewElement("name");
        cur_student_name->SetText(student.name);
        cur_student->InsertEndChild(cur_student_name);

        XMLElement* cur_student_login = xmlDoc.NewElement("login");
        char login[17];
        std::memcpy(login, student.login, 16);
        login[16] = '\0';
        /*size_t ind = 16;
        while (!std::isalpha(login[ind]) && !std::isdigit(login[ind]) && login[ind] != '_') {
            login[ind] = 0;
            --ind;
        }*/
        cur_student_login->SetText(login);
        cur_student->InsertEndChild(cur_student_login);

        XMLElement* cur_student_group = xmlDoc.NewElement("group");
        char group[9];
        std::memcpy(group, student.group, 8);
        group[8] = '\0';
        /*ind = 8;
        while (!std::isalpha(group[ind]) && !std::isdigit(group[ind]) && group[ind] != '_') {
            group[ind] = '\0';
            --ind;
        }*/
        cur_student_group->SetText(group);
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
        cur_student_project_repo->SetText(student.project.repo);
        cur_student_project->InsertEndChild(cur_student_project_repo);

        XMLElement* cur_student_project_mark = xmlDoc.NewElement("mark");
        cur_student_project_mark->SetText(student.project.mark);
        cur_student_project->InsertEndChild(cur_student_project_mark);

        cur_student->InsertEndChild(cur_student_project);

        XMLElement* cur_student_mark = xmlDoc.NewElement("mark");
        cur_student_mark->SetText(student.mark);
        cur_student->InsertEndChild(cur_student_mark);

        root_elem->InsertEndChild(cur_student);
    }
    input.close();
    std::cout << "Read info about " << cnt_students << " student(s)\n";

    xmlDoc.InsertFirstChild(root_elem);
    size_t filename_len = std::strlen(filename);
    filename[filename_len - 3] = 'x';
    filename[filename_len - 2] = 'm';
    filename[filename_len - 1] = 'l';
    XMLError saving_result = xmlDoc.SaveFile(filename);

    if (saving_result == XMLError::XML_SUCCESS) {
        std::cout << "XML successfully generated and saved as " << filename << "\n";
        return;
    }
    std::cerr << "Error " << saving_result << " while saving result XML as " << filename << "\n";
    exit(1);
}

void fromXMLToBin(char* filename) {
    XMLDocument xmlDoc;
    XMLError loading_result = xmlDoc.LoadFile(filename);
    if (loading_result != XMLError::XML_SUCCESS) {
        std::cerr << "Error " << loading_result << " while loading XML from provided file!\n";
        exit(1);
    }
    std::cout << "Reading xml data from " << filename << "\n";

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
        while (ind < 32 && text_ptr[ind] != '\0') {
            student.name[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 32) {
            student.name[ind] = '\0';
            ++ind;
        }

        // login
        text_ptr = cur_student->FirstChildElement("login")->GetText();
        ind = 0;
        while (ind < 16 && text_ptr[ind] != '\0') {
            student.login[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 16) {
            student.login[ind] = '\0';
            ++ind;
        }

        // group
        text_ptr = cur_student->FirstChildElement("group")->GetText();
        ind = 0;
        while (ind < 8 && text_ptr[ind] != '\0') {
            student.group[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 8) {
            student.group[ind] = '\0';
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
        while (ind < 59 && text_ptr[ind] != '\0') {
            student.project.repo[ind] = text_ptr[ind];
            ++ind;
        }
        while (ind < 59) {
            student.project.repo[ind] = '\0';
            ++ind;
        }

        // mark for project
        char* tmp;
        long project_mark = std::strtol(cur_student_project->FirstChildElement("mark")->GetText(), &tmp, 10);
        student.project.mark = static_cast<uint8_t>(project_mark);

        // mark
        student.mark = std::strtof(cur_student->FirstChildElement("mark")->GetText(), &tmp);
        
        output.write(reinterpret_cast<char*>(&student), sizeof student);

        cur_student = cur_student->NextSiblingElement("student");
    }
    output.close();
    std::cout << "Read info about " << cnt_students << " student(s)\n";
    std::cout << "XML successfully parsed to binary format and saved as " << filename << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Expected filename as command line argument!\n";
        return 1;
    }
    char* filename = argv[1];
    size_t filename_len = std::strlen(filename);
    if (filename_len < 4 || 
        (std::strcmp(filename + filename_len - 3, bin_extension) != 0 &&
         std::strcmp(filename + filename_len - 3, xml_extension) != 0)) {
        std::cerr << "Invalid file extension!\n";
        return 1;
    }
    if (std::strcmp(filename + filename_len - 3, bin_extension) == 0) {
        fromBinToXML(filename);
    } else {
        fromXMLToBin(filename);
    }
    return 0;
}