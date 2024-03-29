#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char LOG_BUFFER[2048];

void require(bool cond, char* msg) {
    if (!cond) {
        printf("%s\n", msg);
        exit(1);
    }
}

void requiref(bool cond, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(LOG_BUFFER, format, args);
    va_end(args);
    require(cond, LOG_BUFFER);
}

bool path_exists(char *path) {
    struct stat file;
    return (stat(path, &file) >= 0);
}

void DEBUG(char* msg) {
    char* debug = getenv("DEBUG");
    if (debug == NULL || strcmp(debug, "0") != 0) printf(msg);
}

void DEBUGF(char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(LOG_BUFFER, format, args);
    va_end(args);
    DEBUG(LOG_BUFFER);
}
