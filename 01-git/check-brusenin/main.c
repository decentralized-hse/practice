#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "parser.h"
#include "sha256.h"
#include "utils.h"

struct args {
    char *dir_path;
    char *root_hash;
};

struct args parse_args(int argc, char** argv) {
    require(argc == 3, "wrong number of arguments");
    char* dir_path = argv[1];
    char* root_hash = argv[2];
    require(strlen(root_hash) == 64, "the last argument should be the hash of the root dir (sha256)");
    struct args res = {
        .dir_path = dir_path,
        .root_hash = root_hash,
    };
    return res;
}

bool validate_hash_file(char* hash) {
    requiref(path_exists(hash), "file `%s` doesn't exist", hash);
    char actual_hash[65];
    sha256(hash, actual_hash);
    requiref(strcmp(actual_hash, hash) == 0, "file `%s` has invalid hash:\nactual:   `%s`\nexpected: `%s`", hash, actual_hash, hash);
}

void validate_dir(char* hash) {
    validate_hash_file(hash);

    struct parser p;
    parser_open(&p, hash);
    while (parser_has_next(&p)) {
        char *node_name, *node_hash;
        parser_parse_line(&p, &node_name, &node_hash);
        if (node_name[strlen(node_name)-1] == '/') {
            DEBUGF("[DEBUG] recursively validate %s: %s\n", node_name, node_hash);
            validate_dir(node_hash);
        } else {
            validate_hash_file(node_hash);
        }
    }
    parser_close(&p);
}

char* find_node_hash(char* hash, char* target_node_name) {
    struct parser p;
    parser_open(&p, hash);
    while (parser_has_next(&p)) {
        char *node_name, *node_hash;
        parser_parse_line(&p, &node_name, &node_hash);
        require(strlen(node_name) > 0 &&
            (node_name[strlen(node_name)-1] == '/' || node_name[strlen(node_name)-1] == ':'),
            "failed to parse line: node name has invalid format");
        require(strlen(node_hash) == 64, "failed to parse line: node hash has invalid format");

        if (strlen(node_name) > 0 && node_name[strlen(node_name)-1] == '/') {
            node_name[strlen(node_name)-1] = 0;
        }
        if (strcmp(node_name, target_node_name) == 0) {
            parser_close(&p);
            free(node_name);
            return node_hash;
        }
        free(node_name); free(node_hash);
    }
    parser_close(&p);
    return NULL;
}

char* find_dir_hash(char* root_hash, char* dir_path) {
    if (strcmp(dir_path, "/") == 0 || strcmp(dir_path, ".") == 0) {
        return root_hash;
    }
    char* hash = root_hash;
    char* checkpoint;
    char* dir_path_node = strtok_r(dir_path, "/", &checkpoint);
    while (dir_path_node != NULL) {
        DEBUGF("[DEBUG] dir_path_node=`%s`\n", dir_path_node);
        hash = find_node_hash(hash, dir_path_node);
        requiref(hash != NULL, "failed to find hash for path_node=%s", dir_path_node);
        dir_path_node = strtok_r(NULL, "/", &checkpoint);
    }
    return hash;
}

int main(int argc, char** argv) {
    struct args args = parse_args(argc, argv);
    char* hash = find_dir_hash(args.root_hash, args.dir_path);
    validate_dir(hash);
    printf("directory is valid for given hash\n");
    return 0;
}
