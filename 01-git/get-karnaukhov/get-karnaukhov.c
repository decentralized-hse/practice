#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DIR_SEPARATOR "/\t"
#define FILE_SEPARATOR ":\t"

void parse_arguments(int argc, const char **argv, char **root_hash, char ***path) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <path> <hash>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    *root_hash = strdup(argv[2]);

    char *path_str = strdup(argv[1]);
    int count = 0;
    for (char *token = strtok(path_str, "/"); token != NULL; token = strtok(NULL, "/")) {
        count++;
    }

    free(path_str);
    path_str = strdup(argv[1]);
    *path = (char**)malloc((count + 1) * sizeof(char*));

    int i = 0;
    for (char *token = strtok(path_str, "/"); token != NULL; token = strtok(NULL, "/")) {
        (*path)[i++] = strdup(token);
    }
    (*path)[i] = NULL;
    free(path_str);
}

void update_root_hash(char **root_hash, const char *token, const char *sep) {
    FILE *file = fopen(*root_hash, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", *root_hash);
        exit(EXIT_FAILURE);
    }

    free(*root_hash);
    *root_hash = NULL;

    char *line = NULL;
    size_t line_len = 0;
    ssize_t read = 0;
    while ((read = getline(&line, &line_len, file)) != -1) {
        char *tmp_token = strtok(line, sep);
        char *tmp_root_hash = strtok(NULL, sep);

        if (tmp_token != NULL && tmp_root_hash != NULL && strcmp(tmp_token, token) == 0) {
            *root_hash = strdup(tmp_root_hash);
            (*root_hash)[strlen(*root_hash) - 1] = 0;
            break;
        }
    }

    fclose(file);
    free(line);
}

int main(int argc, const char **argv) {
    char *root_hash = NULL, **path_tokens = NULL;
    parse_arguments(argc, argv, &root_hash, &path_tokens);

    for (int i = 0; path_tokens[i] != NULL; i++) {
        char token[1024] = {0};
        strcpy(token, path_tokens[i]);

        if (access(root_hash, F_OK) != 0) {
            fprintf(stderr, "Non-existing root hash: %s\n", root_hash);
            exit(1);
        }

        const char* sep = DIR_SEPARATOR;
        if (path_tokens[i + 1] == NULL) {
            sep = FILE_SEPARATOR;
        }

        update_root_hash(&root_hash, token, sep);
        if (root_hash == NULL) {
            fprintf(stderr, "Failed to find %s\n", token);
            exit(EXIT_FAILURE);
        }

        if (path_tokens[i + 1] == NULL) {
            FILE *file = fopen(root_hash, "r");
            if (file == NULL) {
                fprintf(stderr, "Error opening file %s\n", root_hash);
                exit(EXIT_FAILURE);
            }

            int c;
            while((c = getc(file)) != EOF) {
                putchar(c);
            }
            fclose(file);
        }
    }

    free(root_hash);
    for (int i = 0; path_tokens[i] != NULL; ++i) {
        free(path_tokens[i]);
    }
    free(path_tokens);

    return 0;
}
