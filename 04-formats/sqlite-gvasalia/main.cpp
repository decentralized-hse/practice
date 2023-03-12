#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>

using namespace std;

struct Project
{
    char repo[59];
    uint8_t mark;

    Project(std::string str, uint8_t m)
    {
        memcpy(repo, str.c_str(), str.size());
        mark = m;
    }

    Project() {}
};

struct Student
{
    char name[32];
    char login[16];
    char group[8];
    uint8_t practice[8];
    Project project;
    float mark;

    Student() {}

    Student(char n[32], char l[16], char g[8], const vector<uint8_t> &p, Project pr, float m)
    {
        memcpy(name, n, 32);
        memcpy(login, l, 16);
        memcpy(group, g, 8);
        for (int i = 0; i < 8; ++i)
            practice[i] = p[i];
        project = pr;
        mark = m;
    }
};

int main(int argc, char *argv[])
{

    string type = argv[1];
    if (type.substr(type.size() - 3, 3) == "bin")
    {
        std::cout << "Reading from binary file\n";
        ifstream file(argv[1], ios::in | ios::binary);

        if (!file.is_open())
        {
            cerr << "Failed to open file\n";
            return 1;
        }

        Student record;
        sqlite3 *db;
        std::string file_name = type.substr(0, type.size() - 4) + ".sqlite";
        int rc = sqlite3_open(file_name.c_str(), &db);
        rc = sqlite3_exec(db, "DROP TABLE IF EXISTS students; ", NULL, NULL, NULL);
        rc = sqlite3_exec(db, "CREATE TABLE students (name TEXT, login TEXT, group1 TEXT, practice TEXT, repo TEXT, mark_project INTEGER, mark FLOAT); ", NULL, NULL, NULL);
        while (file.read((char *)&record, sizeof(Student)))
        {
            if (file.gcount() != sizeof(Student))
            {
                cerr << "Failed to read record\n";
                return 1;
            }

            std::string query = "INSERT INTO students (name, login, group1, practice, repo, mark_project, mark) VALUES ('";
            std::string practice_string = "";
            for (int i = 0; i < 8; ++i)
            {
                practice_string += std::to_string(record.practice[i]);
                if (i != 7)
                {
                    practice_string += ",";
                }
            }

            // sorry :(
            query += record.name;
            query += "', '";
            query += record.login;
            query += "', '";
            query += record.group;
            query += "', '";
            query += practice_string;
            query += "', '";
            query += record.project.repo;
            query += "', '";
            query += to_string(record.project.mark);
            query += "', '";
            query += to_string(record.mark);
            query += "');";
            // sorry again :(

            rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
            std::cout << "Record created successfully in " << file_name << '\n';
        }

        file.close();
        sqlite3_close(db);
    }
    else if (type.substr(type.size() - 7, 7) == ".sqlite")
    {
        std::cout << "Reading from sqlite db\n";
        sqlite3 *db;
        int rc = sqlite3_open(argv[1], &db);
        Student result;
        char get[] = "SELECT name, login, group1, practice, repo, mark_project, mark FROM students;";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, get, -1, &stmt, NULL);
        std::string file_name = type.substr(0, type.size() - 7) + ".bin";
        FILE *out = fopen(file_name.c_str(), "w");
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            std::string name = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
            std::string login = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
            std::string group1 = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
            std::string practice = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
            std::string repo = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)));
            uint8_t mark_project = sqlite3_column_int(stmt, 5);
            float mark = sqlite3_column_double(stmt, 6);

            Student student;
            memcpy(student.name, name.c_str(), 32);
            memcpy(student.login, login.c_str(), 16);
            memcpy(student.project.repo, repo.c_str(), 59);
            memcpy(student.group, group1.c_str(), 8);
            for (int i = 0; i < 8; ++i)
            {
                int num = practice[2 * i];
                student.practice[i] = num - 48;
            }
            student.project.mark = mark_project;
            student.mark = mark;

            fwrite(&student, sizeof(Student), 1, out);
            std::cout << "Record is written to " << file_name << '\n';
        };
        sqlite3_close(db);
        fclose(out);
    }
    else
    {
        cerr << "File should be either .bin of .db\n";
    }
    std::cout << "The end\n";
}
