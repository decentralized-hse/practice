#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>

#define PARENT_RECORD_LEN   74
#define HASH_LEN            64
#define EMPTY_HASH          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

#define OK                  0
#define NOT_ENOUGH_ARGS     1
#define INVALID_NAME        2
#define INVALID_ROOT_HASH   3
#define INVALID_PATH        4
#define ALREADY_EXISTS      5
#define MALFORMED           6
#define CANNOT_OPEN_FILE    7


typedef struct {
    char* path;
    size_t path_len;
    char* root_hash;
} args_t;

int validate_arguments(int argc, char** argv, args_t* res) {
    if (argc < 3) {
        printf("Not enough arguments\n");
        return NOT_ENOUGH_ARGS;
    }

    char* path = argv[1];
    size_t path_len = strlen(path);
    char* root_hash = argv[2];

    if (path_len == 0 || strchr(path, ':') != NULL || strchr(path, '\t') != NULL || path[path_len - 1] == '/') {
        printf("Invalid name\n");
        return INVALID_NAME;
    }

    if (strlen(root_hash) != 64) {
        printf("Invalid root hash\n");
        return INVALID_ROOT_HASH;
    }

    res->path = path;
    res->path_len = path_len;
    res->root_hash = root_hash;
    return OK;
}

void calculate_hash(char* record, char* new_hash) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)record, strlen(record), (unsigned char*)hash);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(new_hash + (i * 2), "%02x", hash[i]);
    }
}

void write_to_buffer(char* smth, char** buffer, int is_hash, int append_slash) {
    unsigned int buffer_len = *buffer == NULL ? 0 : strlen(*buffer);
    unsigned int size = buffer_len + strlen(smth) + (is_hash ? 1 : 2) + 1;

    char* new_buffer = malloc(size);

    if (*buffer != NULL) {
        new_buffer = memcpy(new_buffer, *buffer, buffer_len);
        new_buffer[size - 1] = 0;
        free(*buffer);
    }

    if (is_hash) {
        sprintf(new_buffer + buffer_len, "%s\n", smth);
    } else if (append_slash) {
        sprintf(new_buffer + buffer_len, "%s/\t", smth);
    } else {
        sprintf(new_buffer + buffer_len, "%s\t", smth);
    }

    *buffer = new_buffer;
}

int execute(char* root_hash, char* next_item, char* res_hash, char* path_end) {
    if (next_item == NULL || strlen(next_item) == 0 || next_item >= path_end) {
        printf("Already exists\n");
        return ALREADY_EXISTS;
    }

    size_t next_item_len = strlen(next_item);

    if (next_item_len == 0) {
        printf("Invalid name\n");
        return INVALID_NAME;
    }

    FILE* cur_blob = fopen(root_hash, "r");
    if (cur_blob == NULL) {
        printf("Couldn't open file %s in %s\n", next_item, root_hash);
        return CANNOT_OPEN_FILE;
    }

    char* file_buffer = NULL;

    char* line = NULL;
    size_t len = 0;
    int success = 0;
    while (getline(&line, &len, cur_blob) != -1) {
        char* blob_name = strtok(line, "\t");
        size_t blob_name_len = strlen(blob_name);

        if (blob_name_len == 0) {
            break;
        }

        char* blob_hash = strtok(NULL, "\n");
        if (blob_hash == NULL) {
            printf("Malformed\n");
            return MALFORMED;
        }

        if (strcmp(blob_name, ".parent/") == 0) {
            write_to_buffer(blob_name, &file_buffer, 0, 0);
            write_to_buffer(root_hash, &file_buffer, 1, 0);
        } else if (blob_name[blob_name_len - 1] == '/' && next_item_len + 1 == blob_name_len && strncmp(blob_name, next_item, next_item_len) == 0) {
            char new_hash[HASH_LEN + 1];
            new_hash[HASH_LEN] = 0;

            char* new_next_item = strtok(next_item + strlen(next_item) + 1, "/");
            int sub_res = execute(blob_hash, new_next_item, new_hash, path_end);
            if (sub_res) {
                free(line);
                fclose(cur_blob);
                return sub_res;
            }

            write_to_buffer(blob_name, &file_buffer, 0, 0);
            write_to_buffer(new_hash, &file_buffer, 1, 0);
            success = 1;
        } else if (strncmp(blob_name, next_item, next_item_len) > 0 && !success) {
            if (next_item + strlen(next_item) + 1 < path_end) {
                printf("Parent directory %s does not exist\n", next_item);
                return INVALID_PATH;
            }

            FILE* new_dir = fopen(EMPTY_HASH, "w");
            if (new_dir == NULL) {
                printf ("Could not open file to create empty directory with hash %s", EMPTY_HASH);
                return CANNOT_OPEN_FILE;
            }
            fclose(new_dir);

            write_to_buffer(next_item, &file_buffer, 0, 1);
            write_to_buffer(EMPTY_HASH, &file_buffer, 1, 0);

            write_to_buffer(blob_name, &file_buffer, 0, 0);
            write_to_buffer(blob_hash, &file_buffer, 1, 0);
            success = 1;
        } else {
            write_to_buffer(blob_name, &file_buffer, 0, 0);
            write_to_buffer(blob_hash, &file_buffer, 1, 0);
        }
    }

    if (!success) {
        if (next_item + strlen(next_item) + 1 < path_end) {
            printf("Invalid path\n");
            return INVALID_PATH;
        }

        FILE* new_dir = fopen(EMPTY_HASH, "w");
        if (new_dir == NULL) {
            printf ("Could not open file to create empty directory with hash %s", EMPTY_HASH);
            return CANNOT_OPEN_FILE;
        }
        fclose(new_dir);

        write_to_buffer(next_item, &file_buffer, 0, 1);
        write_to_buffer(EMPTY_HASH, &file_buffer, 1, 0);
    }


    if (file_buffer) {
        calculate_hash(file_buffer, res_hash);

        FILE* out = fopen(res_hash, "w"); 
        if (out == NULL) {
            printf ("Could not open file to create create new version of directory with hash %s", res_hash);
            return CANNOT_OPEN_FILE;
        }
        fprintf(out, "%s", file_buffer); 
        fclose(out);

        free(file_buffer);

        success = 1;
    }

    if (line) {
        free(line);
    }

    if (success) {
        return OK;
    }

    printf("Invalid path\n");
    return INVALID_PATH;
}

int main(int argc, char** argv) {
    args_t args;

    int args_error = validate_arguments(argc, argv, &args);
    if (args_error) {
        return args_error;
    }

    char new_hash[HASH_LEN + 1];
    new_hash[HASH_LEN] = 0;

    char* next_item = strtok(args.path, "/");

    int res = execute(args.root_hash, next_item, new_hash, args.path + args.path_len);
    if (res != 0) {
        return res;
    }

    printf("%s\n", new_hash);

    return OK;
}
