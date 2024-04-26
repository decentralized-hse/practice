#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <stdbool.h>

#define MAX_STR 4096

typedef struct fileEntry {
    char name[MAX_STR];
    char hash[65]; // SHA-256 hash string
    bool is_dir;
} FileEntry;

typedef struct directory {
    FileEntry *entries;
    int count;
} Directory;

Directory readDirectory(const char *dirPath) {
    FILE *file = fopen(dirPath, "r");
    assert(file != NULL);


    Directory dir = {NULL, 0};

    char current_line[MAX_STR];
    while (fgets(current_line, sizeof(current_line), file) != NULL) {
        char tmp_check[MAX_STR];
        snprintf(tmp_check, MAX_STR, "%s", current_line);
        strtok(tmp_check, "\t");

        if (strcmp(".parent/", tmp_check) == 0 || strcmp(".commit:", tmp_check) == 0){
            continue;
        }

        dir.entries = realloc(dir.entries, sizeof(FileEntry) * (dir.count + 1));

        char tmp[MAX_STR];
        snprintf(tmp, MAX_STR, "%s", current_line);
        strtok(tmp, ":");
        if (strtok(NULL, ":") == NULL) {
            char tmp_name[MAX_STR];
            snprintf(tmp_name, MAX_STR, "%s", current_line);
            snprintf(dir.entries[dir.count].name, MAX_STR, "%s", strtok(tmp_name, "/"));

            char tmp_hash[MAX_STR];
            snprintf(tmp_hash, MAX_STR, "%s", current_line);
            strtok(tmp_hash, "\t");
            snprintf(dir.entries[dir.count].hash, 65, "%s", strtok(NULL, "\t"));

            dir.entries[dir.count].is_dir = true;
        } else {
            char tmp_name[MAX_STR];
            snprintf(tmp_name, MAX_STR, "%s", current_line);
            snprintf(dir.entries[dir.count].name, MAX_STR, "%s", strtok(tmp_name, ":"));

            char tmp_hash[MAX_STR];
            snprintf(tmp_hash, MAX_STR, "%s", current_line);
            strtok(tmp_hash, "\t");
            snprintf(dir.entries[dir.count].hash, 65, "%s", strtok(NULL, "\t"));

            dir.entries[dir.count].is_dir = false;
        }
        dir.count++;
    }

    fclose(file);
    return dir;
}

void diffDirectories(const char *prevHash, const char *newHash, const char *path, char *diff) {
    Directory prevDir = readDirectory(prevHash);
    Directory newDir = readDirectory(newHash);

    for (int i = 0; i < prevDir.count; ++i) {
        if (prevDir.entries[i].is_dir) {
            continue;
        }
        bool found = false;
        bool is_dir = false;
        for (int j = 0; j < newDir.count; ++j) {
            if (strcmp(newDir.entries[j].name, prevDir.entries[i].name) != 0) {
                continue;
            }
            found = true;
            if (newDir.entries[j].is_dir) {
                is_dir = true;
                break;
            }
            if (strcmp(newDir.entries[j].hash, prevDir.entries[i].hash) != 0) {
                char line_changed[MAX_STR];
                snprintf(line_changed, MAX_STR, "- %s/%s\n", path, prevDir.entries[i].name);
                strcat(diff, line_changed);
            }
            newDir.entries[j] = newDir.entries[newDir.count - 1];
            --newDir.count;
            break;
        }
        if (is_dir) {
            continue;
        }
        if (!found) {
            char line_remove[MAX_STR];
            snprintf(line_remove, MAX_STR, "- %s/%s\n", path, prevDir.entries[i].name);
            strcat(diff, line_remove);
        }
    }

    for (int j = 0; j < newDir.count; ++j) {
        if (newDir.entries[j].is_dir) {
            continue;
        }
        char line_add[MAX_STR];
        snprintf(line_add, MAX_STR, "+ %s/%s\n", path, newDir.entries[j].name);
        strcat(diff, line_add);

        newDir.entries[j] = newDir.entries[newDir.count - 1];
        --newDir.count;
        j = 0;
    }

//    for (int j = 0; j < newDir.count; ++j) {
//        if (!newDir.entries[j].is_dir) {
//            continue;
//        }
//        char line_add[MAX_STR];
//        snprintf(line_add, MAX_STR, "d %s/%s\n", path, newDir.entries[j].name);
//        strcat(diff, line_add);
//
//        newDir.entries[j] = newDir.entries[newDir.count - 1];
//        --newDir.count;
//        j = 0;
//    }

    for (int i = 0; i < prevDir.count; ++i) {
//        if (!prevDir.entries[i].is_dir) {
//            continue;
//        }

        FileEntry prevEntry = prevDir.entries[i];
        FileEntry newEntry;

//        bool found = false;
        for (int j = 0; j < newDir.count; ++j) {
            if (strcmp(newDir.entries[j].name, prevDir.entries[i].name) != 0) {
                continue;
            }
//            found = true;
            newEntry = newDir.entries[j];
            break;
        }

        if (prevEntry.is_dir && newEntry.is_dir) {
            char newPath[MAX_STR];
            snprintf(newPath, MAX_STR, "%s/%s", path, prevEntry.name);

            char extraDiff[MAX_STR];
            diffDirectories(prevEntry.hash, newEntry.hash, newPath, extraDiff);

            if (strcmp(extraDiff, "") != 0) {
                char line_dir[MAX_STR];
                snprintf(line_dir, MAX_STR, "d %s/%s\n", path, prevEntry.name);
                strcat(diff, line_dir);
                strcat(diff, extraDiff);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "wrong arguments\n");
        return 1;
    }

    char* path = argv[1];
    char* prevHash = argv[2];
    char* newHash = argv[3];

    char diff[MAX_STR] = "";
    diffDirectories(prevHash, newHash, path, diff);
    printf("%s\n", diff);

    return 0;
}