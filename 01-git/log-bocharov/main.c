#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define HASH_LENGTH 64
#define HASH_BUFFER_SIZE 65
#define MAX_PATH_LENGTH 1024 
#define MAX_DATA_FILE_SIZE  4096

const char* commitPrefix = ".commit:\t";
const char* parentPrefix = ".parent/\t";
const char* rootPrefix = "Root:";
const char* datePrefix = "Date:";


int ReadFile(const char* path, char* dest, size_t size) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("Failed to open %s", path);
        return 1;
    }
    char line[256];
    int currentPosition = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        int lineLength = strlen(line);
        if (currentPosition + lineLength < size) {
            strcpy(dest + currentPosition, line);
            currentPosition += lineLength;
        } else {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

int IsValidHex(const char* s) {
    int length = strlen(s);
    if (length != 64) {
        return 1;
    }

    for (int i = 0; i < length; i++) {
        if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) {
            return 1;
        }
    }

    return 0;
}

int GetParentAndCommitFromRoot(const char* path, char* commit, char* parent) {
    commit[0] = '\0';
    parent[0] = '\0';
    char rootData[MAX_DATA_FILE_SIZE];
    if (ReadFile(path, rootData, MAX_DATA_FILE_SIZE)) {
        return 1;
    }

    char* data = rootData;
    while (*data) {
        if (commit[0] == '\0' && strncmp(data, commitPrefix, strlen(commitPrefix)) == 0) {
            strncpy(commit, data + strlen(commitPrefix), HASH_LENGTH);
            commit[HASH_LENGTH] = '\0';
        } else if (parent[0] == '\0' &&strncmp(data, parentPrefix, strlen(parentPrefix)) == 0) {
            strncpy(parent, data + strlen(parentPrefix), HASH_LENGTH);
            parent[HASH_LENGTH] = '\0';
        }

        while (*data && *data != '\n') data++;
        if (*data) data++;
    }

    if (IsValidHex(commit)) {
        commit[0] = '\0';
    }
    if (IsValidHex(parent)) {
        parent[0] = '\0';
    }
    return 0;
}

void PrintCommit(const char* path, char* prevRoot) {
    char data[MAX_DATA_FILE_SIZE];
    if (ReadFile(path, data, sizeof(data))) {
        return;
    }
    char* ptr = data;
    int first_line_size = strcspn(ptr, "\n\0");
    if (first_line_size != strlen(rootPrefix) + HASH_LENGTH + 1) {
        return;
    }
    if (strncmp(ptr, rootPrefix, strlen(rootPrefix))) {
        return;
    }
    ptr += strlen(rootPrefix);
    ptr += 1;

    char currRoot[HASH_BUFFER_SIZE];
    strncpy(currRoot, ptr, HASH_LENGTH);
    currRoot[HASH_LENGTH] = '\0';
    if (strcmp(currRoot, prevRoot) == 0) {
        return;
    }
    ptr += HASH_LENGTH + 1;

    if (strncmp(ptr, datePrefix, strlen(datePrefix))) {
        return;
    }
    
    strcpy(prevRoot, currRoot);
    printf("%s", data);
}


int main(int argc, char* argv[]) {
    if (argc < 2 || strlen(argv[1]) != HASH_LENGTH) {
        fprintf(stderr, "No hash in args or hash size not equal 64. Exit.");
        return 1;
    }
    char fsRootPath[MAX_PATH_LENGTH];
    if (getcwd(fsRootPath, sizeof(fsRootPath)) == NULL) {
        return 1;
    }

    char rootPath[MAX_PATH_LENGTH * 2];
    snprintf(rootPath, sizeof(rootPath), "%s/%s", fsRootPath, argv[1]);

    char commit[HASH_BUFFER_SIZE];
    char parent[HASH_BUFFER_SIZE];
    char prevCommitRoot[HASH_BUFFER_SIZE] = {'\0'};

    while (!GetParentAndCommitFromRoot(rootPath, commit, parent)) {
        if (strlen(commit) == HASH_LENGTH) {
            char commitPath[MAX_PATH_LENGTH * 2];
            snprintf(commitPath, sizeof(commitPath), "%s/%s", fsRootPath, commit);
            PrintCommit(commit, prevCommitRoot);
        }
        if (strlen(parent) == 0) {
            break;
        }
        snprintf(rootPath, sizeof(rootPath), "%s/%s", fsRootPath, parent);

    }

    return 0;
}
