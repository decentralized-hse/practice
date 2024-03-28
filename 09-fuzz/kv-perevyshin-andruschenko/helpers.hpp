#include <fstream>
#include <iostream>
#include <vector>
#include <charconv>
#include <string>
#include <cstdio>
#include <cstring>
#include <regex>
#include <string_view>
#include <array>

struct Student {
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
        char    repo[63];
        uint8_t mark;
    } project;

    // 32 bit IEEE 754 float
    float   mark; 
};


class MalformedInputException: public std::exception {
public:
    const char* what() {
        return "Malformed input";
    }
};


// Украдено отсюда: http://www.zedwood.com/article/cpp-is-valid-utf8-string-function
// Предварительно провалидировано
bool utf8_check_is_valid(std::string_view string) {
    int c,i,ix,n,j;
    for (i=0, ix=string.length(); i < ix; i++)
    {
        c = (unsigned char) string[i];
        //if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
        if (0x00 <= c && c <= 0x7f) n=0; // 0bbbbbbb
        else if ((c & 0xE0) == 0xC0) n=1; // 110bbbbb
        else if ( c==0xed && i<(ix-1) && ((unsigned char)string[i+1] & 0xa0)==0xa0) return false; //U+d800 to U+dfff
        else if ((c & 0xF0) == 0xE0) n=2; // 1110bbbb
        else if ((c & 0xF8) == 0xF0) n=3; // 11110bbb
        //else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return false;
        for (j=0; j<n && i<ix; j++) { // n bytes matching 10bbbbbb follow ?
            if ((++i == ix) || (( (unsigned char)string[i] & 0xC0) != 0x80))
                return false;
        }
    }
    return true;
}

bool is_name_valid(const char* name) {
    for (size_t i = 0, zeroes = 0; i < 32; ++i) {
        if (zeroes && name[i] != 0) {
            return false;
        }
        if (!zeroes && name[i] == 0) {
            zeroes = 1;
        }
    }

    std::string_view name_str(name, 32);
    return utf8_check_is_valid(name_str);
}

bool is_student_valid(const Student* student) {
    bool name_check = is_name_valid(student->name);
    if (!name_check) {
        return false;
    }

    std::regex ascii_regex("[\\w]+");

    bool login_check = std::regex_match(std::string(std::string_view(student->login, 16)), ascii_regex);
    bool group_check = std::regex_match(std::string(std::string_view(student->group, 8)), ascii_regex);
    if (!login_check || !group_check) {
        return false;
    }

    for (size_t prac = 0; prac < 8; ++prac) {
        if (student->practice[prac] != 0 && student->practice[prac] != 1) {
            return false;
        }
    }

    bool repo_check = std::regex_match(std::string(std::string_view(student->project.repo, 63)), ascii_regex);
    if (!repo_check) {
        return false;
    }

    return true;
}

bool is_input_valid(const uint8_t *data, size_t size) {
    if (size % sizeof(Student) != 0) {
        return false;  
    }

    size_t num_records = size / sizeof(Student);
    const Student* students = reinterpret_cast<const Student*>(data);

    for (size_t i = 0; i < num_records; ++i) {
        const Student* student = students + i;

        if (!is_student_valid(student)) {
            return false;
        }
    }

    return true;
}
