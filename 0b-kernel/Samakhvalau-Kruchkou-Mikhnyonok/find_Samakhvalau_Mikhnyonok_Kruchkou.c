#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
#include <unistd.h>

#define MAX_PATH 4096
#define MAX_LINE 2048

typedef struct {
    const char *name;
    const char *pattern_str;
    regex_t regex;
    int weight;
} Pattern;

typedef struct {
    char *filepath;
    int score;
} FileResult;

FileResult *results = NULL;
size_t results_count = 0;
size_t results_capacity = 0;

Pattern patterns[] = {
    {"Bitwise Hacks", "(<<|>>|& [_a-zA-Z0-9]+)", {}, 1},
    {"Memory Pooling", "(mem_pool|blob_alloc|xcalloc)", {}, 1},
    {"Inline/Preload", "(inline|__attribute__\\s*\\(\\s*\\(always_inline\\)\\)|prefetch)", {}, 1},
    {"Struct Packing", "(__attribute__\\s*\\(\\s*\\(?\\s*packed\\s*\\)?\\s*\\)|#pragma pack|__packed|PACKED)", {}, 1},
    {"Cache Alignment", "(__attribute__\\s*\\(\\s*\\(?\\s*aligned\\s*\\(\\s*64\\s*\\)\\s*\\)|__cacheline_aligned|CACHELINE_SIZE|aligned\\(CACHE_LINE\\)|__aligned\\(64\\))", {}, 1},
    {"Explicit Padding", "(char\\s+(pad|reserved|_padding)\\[[0-9]+\\]|unsigned\\s+(char|int)\\s+padding\\[[0-9]+\\])", {}, 1},
    {"Custom Data Structs", "(ewah_|bitmap|hashmap_entry)", {}, 1},
    {"Dev Comments", "(/\\*.*(optimize|speed|fast path|performance|hack).*\\*/)", {}, 2},
    {"Syscall Avoidance", "(lstat|mmap|O_NOATIME)", {}, 1}
};

#define PATTERN_COUNT (sizeof(patterns) / sizeof(patterns[0]))

void compile_patterns() {
    for (size_t i = 0; i < PATTERN_COUNT; i++) {
        if (regcomp(&patterns[i].regex, patterns[i].pattern_str, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
            exit(1);
        }
    }
}

void free_patterns() {
    for (size_t i = 0; i < PATTERN_COUNT; i++) {
        regfree(&patterns[i].regex);
    }
}

void add_result(const char *path, int score) {
    if (results_count == results_capacity) {
        results_capacity = (results_capacity == 0) ? 1024 : results_capacity * 2;
        results = realloc(results, results_capacity * sizeof(FileResult));
        if (!results) { perror("realloc"); exit(1); }
    }
    results[results_count].filepath = strdup(path);
    results[results_count].score = score;
    results_count++;
}

void analyze_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return;

    char *line = NULL;
    size_t len = 0;
    int score = 0;

    while (getline(&line, &len, fp) != -1) {
        for (size_t i = 0; i < PATTERN_COUNT; i++) {
            if (regexec(&patterns[i].regex, line, 0, NULL, 0) == 0) {
                score += patterns[i].weight;
            }
        }
    }

    if (score > 0) {
        add_result(filepath, score);
    }

    free(line);
    fclose(fp);
}

int is_source_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return 0;
    return (strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0);
}

void process_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    char path[MAX_PATH];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (strcmp(entry->d_name, ".git") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat statbuf;
        if (lstat(path, &statbuf) == -1) continue;

        if (S_ISDIR(statbuf.st_mode)) {
            process_directory(path);
        } else if (S_ISREG(statbuf.st_mode)) {
            if (is_source_file(entry->d_name)) {
                analyze_file(path);
            }
        }
    }
    closedir(dir);
}

int compare_results(const void *a, const void *b) {
    FileResult *ra = (FileResult *)a;
    FileResult *rb = (FileResult *)b;
    return rb->score - ra->score;
}

int main(int argc, char *argv[]) {
    const char *root_dir = (argc > 1) ? argv[1] : ".";

    compile_patterns();
    process_directory(root_dir);

    qsort(results, results_count, sizeof(FileResult), compare_results);

    int limit = (results_count < 10) ? results_count : 10;
    
    for (int i = 0; i < limit; i++) {
        printf("[%d] File: %s\n", results[i].score, results[i].filepath);
        printf("----------------------------------------\n");
    }

    for (size_t i = 0; i < results_count; i++) {
        free(results[i].filepath);
    }
    free(results);
    free_patterns();

    return 0;
}

