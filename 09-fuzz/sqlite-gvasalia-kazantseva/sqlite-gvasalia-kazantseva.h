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

struct Project {
    char repo[59];
    uint8_t mark;

    Project(std::string str, uint8_t m)
    {
        memcpy(repo, str.c_str(), str.size());
        mark = m;
    }

    Project() {}
};


struct Student {
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


size_t GetNewSize(const char* data, size_t size) {
    size_t count = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\'') {
            ++count;
        }
    };
    return count;
}


void PrepareStringForSql(const char* oldData, char* newData, size_t size) {
    size_t j = 0;

    for (size_t i = 0; i < size; ++i) {
        if (oldData[i] == '\'') {
            newData[j] = '\'';
            ++j;
            newData[j] = '\'';
            ++j;
        } else {
            newData[j] = oldData[i];
            ++j;
        }
    }

    std::string end = "', '";
    for (size_t i = 0; i < end.size(); ++i) {
        newData[j++] = end[i];
    }
    newData[j++] = '\0';
    return;
}


void ConvertBinaryDataIntoSqlite(const char * path) {
    std::string type = path;
    std::cout << "Reading from binary file\n";
    ifstream file(path, ios::in | ios::binary);

    if (!file.is_open()) {
        cerr << "Failed to open file\n";
        return;
    }

    Student record;
    sqlite3 *db;
    std::string file_name = type.substr(0, type.size() - 4) + ".sqlite";
    int rc = sqlite3_open(file_name.c_str(), &db);
    rc = sqlite3_exec(db, "DROP TABLE IF EXISTS students; ", NULL, NULL, NULL);
    rc = sqlite3_exec(db, "CREATE TABLE students (name TEXT, login TEXT, group1 TEXT, practice TEXT, repo TEXT, mark_project INTEGER, mark TEXT); ", NULL, NULL, NULL);
    while (file.read((char *)&record, sizeof(Student)))
    {
        if (file.gcount() != sizeof(Student))
        {
            cerr << "Failed to read record\n";
            return;
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
        std::string end = "', '";
        size_t formattedStringSize = 32 + GetNewSize(record.name, 32) + end.size() + 1;
        char formattedStringName[formattedStringSize];
        PrepareStringForSql(record.name, formattedStringName, 32);
        query += formattedStringName;

        if (std::string(formattedStringName).size() + 1 != formattedStringSize) {
            query += "', '";
        }

        formattedStringSize = 16 + GetNewSize(record.login, 16) + end.size() + 1;
        char formattedStringLogin[formattedStringSize];
        PrepareStringForSql(record.login, formattedStringLogin, 16);
        query += formattedStringLogin;
        if (std::string(formattedStringLogin).size() + 1 != formattedStringSize) {
            query += "', '";
        }


        formattedStringSize = 8 + GetNewSize(record.group, 8) + end.size() + 1;
        char formattedStringGroup[formattedStringSize];
        PrepareStringForSql(record.group, formattedStringGroup, 8);
        query += formattedStringGroup;
        if (std::string(formattedStringGroup).size() + 1 != formattedStringSize) {
            query += "', '";
        }

        query += practice_string;
        query += "', '";

        formattedStringSize = 59 + GetNewSize(record.project.repo, 59) + end.size() + 1;
        char formattedStringRecordProjectRepo[formattedStringSize];
        PrepareStringForSql(record.project.repo, formattedStringRecordProjectRepo, 59);
        query += formattedStringRecordProjectRepo;
        if (std::string(formattedStringRecordProjectRepo).size() + 1 != formattedStringSize) {
            query += "', '";
        }

        query += to_string(record.project.mark);
        query += "', '";

        std::ostringstream out;
        out << std::scientific << std::setprecision(std::numeric_limits<double>::max_digits10) << double(record.mark);
        std::string markStr = out.str();
        query += markStr;
        query += "');";

        char *zErrMsg = 0;
        rc = sqlite3_exec(db, query.c_str(), NULL, NULL, &zErrMsg);

        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }

        std::cout << "Record created successfully in " << file_name << '\n';
    }

    file.close();
    sqlite3_close(db);
};


void ConvertSqliteDataIntoBinary(const char* path, const char* new_path) {
    std::string type = path;
    std::cout << "Reading from sqlite db\n";

    sqlite3 *db;
    int rc = sqlite3_open(path, &db);
    Student result;
    char get[] = "SELECT name, login, group1, practice, repo, mark_project, mark FROM students;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, get, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
      fprintf(stderr, "Failed to prepare statement\n");
      fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(db));

    }

    std::string file_name = new_path;
    FILE *out = fopen(file_name.c_str(), "w");
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string name = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
        std::string login = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        std::string group1 = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        std::string practice = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        std::string repo = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)));
        uint8_t mark_project = sqlite3_column_int(stmt, 5);

        const unsigned char *mark_string = sqlite3_column_text(stmt, 6);
        double mark = std::stod(reinterpret_cast<const char*>(mark_string));

        Student student;
        memset(&student, 0, sizeof(Student));
        memcpy(student.name, name.c_str(), min(name.size() + 1, sizeof(student.name)));
        memcpy(student.login, login.c_str(), min(login.size() + 1, sizeof(student.login)));
        memcpy(student.project.repo, repo.c_str(), min(repo.size() + 1, sizeof(student.project.repo)));
        memcpy(student.group, group1.c_str(), min(group1.size() + 1, sizeof(student.group)));
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


    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    fclose(out);
}
