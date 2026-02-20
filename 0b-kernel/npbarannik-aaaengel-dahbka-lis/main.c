#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 16384
#define MAX_PATH 512
#define MAX_COMMITS 8192
#define TOP_N 5

typedef struct {
    char path[MAX_PATH]; int count;
} Entry;

static Entry table[MAX_FILES];
static int ntable;

static char commits[MAX_COMMITS][41]; // 40-char SHA-1 + NUL
static int ncommits = 0;

static void tally(const char *path) {
    for (int i = 0; i < ntable; i++) {
        if (strcmp(table[i].path, path) == 0) {
            table[i].count++;
            return;
        }
    }

    if (ntable < MAX_FILES) {
        strncpy(table[ntable].path, path, MAX_PATH - 1);
        table[ntable].path[MAX_PATH - 1] = '\0';
        table[ntable].count = 1;
        ntable++;
    }
}

static int cmp_desc(const void *a, const void *b) {
    return ((const Entry *)b)->count - ((const Entry *)a)->count;
}

static int safe_path(const char *s) {
    for (; *s; s++) {
        if (strchr(";|&`$(){}[]<>!#~\\", *s)) {
            return 0;
        }
    }

    return 1;
}

static int resolve(const char *arg, char *out, size_t n) {
    char cmd[MAX_PATH + 64];
    snprintf(cmd, sizeof cmd,"git ls-files --full-name -- \"%s\" 2>/dev/null", arg);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }

    int ok = fgets(out, (int)n, fp) != NULL;
    pclose(fp);

    if (ok) {
        out[strcspn(out, "\n")] = '\0';
    }

    return ok && out[0] != '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: @%s <file>\n", argv[0]);
        return 1;
    }

    if (!safe_path(argv[1])) {
        fprintf(stderr, "error: invalid characters in path\n");
        return 1;
    }

    char target[MAX_PATH];
    char cmd[MAX_PATH + 128];

    // 1. Resolve supplied path to a canonical repo-relative path
    if (!resolve(argv[1], target, sizeof target)) {
        fprintf(stderr, "error: '%s' is not tracked by git\n", argv[1]);
        return 1;
    }

    // 2. Collect every (non-merge) commit that touched this file
    snprintf(cmd, sizeof cmd, "git log --no-merges --follow --format=%%H -- \"%s\" 2>/dev/null", target);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen"); return 1;
    }

    char line[64];
    while (ncommits < MAX_COMMITS && fgets(line, sizeof line, fp)) {
        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) == 40) {
            memcpy(commits[ncommits++], line, 41);
        }
    }

    pclose(fp);

    if (ncommits == 0) {
        fprintf(stderr, "no commits found for: %s\n", target);
        return 1;
    }

    // 3. For each commit enumerate all changed files and tally counts.
    char path[MAX_PATH];
    for (int i = 0; i < ncommits; i++) {
        snprintf(cmd, sizeof cmd, "git diff-tree --no-commit-id -r --name-only %s 2>/dev/null", commits[i]);
        fp = popen(cmd, "r");
        if (!fp) {
            continue;
        }

        while (fgets(path, sizeof path, fp)) {
            path[strcspn(path, "\n")] = '\0';
            if (path[0] != '\0' && strcmp(path, target) != 0) {
                tally(path);
            }
        }

        pclose(fp);
    }

    // 4. Sort and print the top N results.
    if (ntable == 0) {
        printf("'%s' was never committed together with another file.\n", target);
        return 0;
    }

    qsort(table, ntable, sizeof *table, cmp_desc);

    int top = ntable < TOP_N ? ntable : TOP_N;
    printf("Top %d files co-changed with \"%s\"  (%d commit%s scanned)\n\n", top, target, ncommits, ncommits == 1 ? "" : "s");

    for (int i = 0; i < top; i++) {
        printf("  %d.  %-60s  %dx\n", i + 1, table[i].path, table[i].count);
    }

    return 0;
}
