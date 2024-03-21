#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <cctype>
#include <cstdlib>

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

void fromXMLToBin(const char* filename_);

bool fromBinToXML(const char* filename_);