#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

struct parser {
    char* file_path;

    FILE* _file;
    char* _line;
    size_t _len;
    bool _done;
};

static void _advance(struct parser* p) {
    if (p->_done) {
        return;
    }
    ssize_t read = getline(&p->_line, &p->_len, p->_file);
    if (read == -1) {
        p->_done = true;
        return;
    }
}

void parser_open(struct parser* p, char* file_path) {
    p->file_path = file_path;

    FILE* fp = fopen(p->file_path, "r");
    require(fp != NULL, "failed to open file");
    p->_file = fp;
    p->_line = NULL;
    p->_len = 0;
    _advance(p);
}

bool parser_has_next(struct parser* p) {
    return !p->_done;
}

bool parser_parse_line(struct parser* p, char** node_name, char** node_hash) {
    DEBUGF("[DEBUG] parse line: %s", p->_line);
    char *checkpoint;
    *node_name = strdup(strtok_r(p->_line, "\t", &checkpoint));
    require(*node_name != NULL, "failed to parse hash file: node name is null");
    *node_hash = strdup(strtok_r(NULL, "\n", &checkpoint));
    require(*node_hash != NULL, "faild to parse hash file: node hash is null");
    DEBUGF("[DEBUG] > node_name=`%s`\tnode_hash=`%s`\n", *node_name, *node_hash);
    _advance(p);
}

bool parser_close(struct parser* p) {
    fclose(p->_file);
    if (p->_line) free(p->_line);
}
