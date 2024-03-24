#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <openssl/sha.h>
#include <unistd.h>


#define HASH_LEN (SHA256_DIGEST_LENGTH * 2)
#define MAX_COMMIT_MESSAGE_LENGTH 1024
#define MAX_DIR_SIZE 1024
#define MAX_DIR_LINE_LENGTH 1024
#define COMMIT_PREFIX ".commit:\t"
#define PARENT_PREFIX ".parent/\t"

static char dig_to_hex(uint8_t dig) {
  if (dig < 10) {
    return '0' + dig;
  }
  return 'a' + dig - 10;
}

static void hash_to_digest(unsigned char* hash, size_t len) {
  for (size_t i = len; i > 0; --i) {
    uint8_t byte = hash[i - 1];
    hash[i * 2 - 2] = dig_to_hex(byte >> 4);
    hash[i * 2 - 1] = dig_to_hex(byte & 15);
  }
}

int calc_sha256(const char *filename, char *hash) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s for SHA256 calculation:", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *file_data = (unsigned char *)malloc(file_size);
    if (!file_data) {
        fprintf(stderr, "Error allocating memory for SHA256 calculation");
        fclose(file);
        return 1;
    }

    if (fread(file_data, 1, file_size, file) != file_size) {
        fprintf(stderr, "Error reading file for SHA256 calculation");
        free(file_data);
        fclose(file);
        return 1;
    }

    fclose(file);

    SHA256(file_data, file_size, hash);
    free(file_data);
    hash_to_digest(hash, SHA256_DIGEST_LENGTH);
    return 0;
}

int create_commit_file(const char *root_hash, const char *commit_message, char *commit_hash) {
    char tmp_commit_file_name[] = "tmp_commit_filename";

    int commit_fd = open(tmp_commit_file_name,  O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (commit_fd < 0) {
        fprintf(stderr, "failed to create temporary commit object: %s\n", strerror(errno));
        return -1;
    }

    FILE* stream = fdopen(commit_fd, "w");
    fprintf(stream, "Root:\t%s\n", root_hash);

    time_t t = time(NULL);
    struct tm *tm_info;
    tm_info = localtime(&t);
    char time_buffer[80];
    strftime(time_buffer, 80, "%d %b %Y %H:%M:%S %Z", tm_info);
    fprintf(stream, "Date:\t%s\n", time_buffer);
    fprintf(stream, "\n%s", commit_message);

    fclose(stream);
    close(commit_fd);

    calc_sha256(tmp_commit_file_name, commit_hash);

    commit_hash[HASH_LEN] = '\0';

    return rename(tmp_commit_file_name, commit_hash);
}

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

int create_new_root_file(const char* root_hash, const char* commit_hash, char* new_root_hash) {
    char tmp_root_file_name[] = "tmp_route_file";

    FILE *file = fopen(root_hash, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening root file: %s\n", root_hash);
        return -1;
    }

    char line[MAX_DIR_LINE_LENGTH];
    char *lines[MAX_DIR_SIZE];
    size_t line_count = 0;

    // Read lines from file
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, COMMIT_PREFIX, strlen(COMMIT_PREFIX)) != 0 &&
            strncmp(line, PARENT_PREFIX, strlen(PARENT_PREFIX)) != 0) {
            lines[line_count] = strdup(line);
            line_count++;
        }
    }
    fclose(file);

    // Add commit line
    snprintf(line, sizeof(line), "%s%s\n", COMMIT_PREFIX, commit_hash);
    lines[line_count] = strdup(line);
    line_count++;

    // Add parent line
    snprintf(line, sizeof(line), "%s%s\n", PARENT_PREFIX, root_hash);
    lines[line_count] = strdup(line);
    line_count++;

    // Sort lines
    qsort(lines, line_count, sizeof(char*), compare_strings);

    int root_fd = open(tmp_root_file_name,  O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (root_fd < 0) {
        fprintf(stderr, "Failed to create temporary root object: %s\n", strerror(errno));
        return -1;
    }

    FILE* stream = fdopen(root_fd, "w");
    for (size_t i = 0; i < line_count; i++) {
        fprintf(stream, "%s", lines[i]);
        free(lines[i]);
    }
    fclose(stream);
    close(root_fd);
    
    calc_sha256(tmp_root_file_name, new_root_hash);
    new_root_hash[HASH_LEN] = '\0';
    return rename(tmp_root_file_name, new_root_hash);
}

int main(int argc, char** argv) {
    if (argc != 2 || strlen(argv[1]) != HASH_LEN) {
        printf("Invalid arguments. Expected: commit <256bit-hash>\n");
        return 1;
    }

    printf("Insert your commit message:\n");

    char commit_message[MAX_COMMIT_MESSAGE_LENGTH];

    if (fgets(commit_message, sizeof(commit_message), stdin) == NULL) {
        printf("failed to read commit comment from stdin\n");
        return 1;
    }

    // create commit file
    char commit_hash[HASH_LEN + 1];
    if (create_commit_file(argv[1], commit_message, commit_hash) != 0) {
        printf("Failed to create commit file\n");
    }

    // create new root file
    char new_root_hash[HASH_LEN + 1];
    if (create_new_root_file(argv[1], commit_hash, new_root_hash) != 0) {
        printf("Failed to create new root file\n");
    }

    // print results
    char commit_hash_short[8];
    char new_root_hash_short[8];
    memcpy (&commit_hash_short, &commit_hash, 7);
    memcpy (&new_root_hash_short, &new_root_hash, 7);
    commit_hash_short[7] = '\0';
    new_root_hash_short[7] = '\0';
    printf("[%s] %s", commit_hash_short, commit_message);
    printf("New root: %s\n", new_root_hash_short);
    return 0;
}