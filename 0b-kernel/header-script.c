#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_AUTHORS 1000
#define MAX_LINE 512

typedef struct {
    char email[256];
    int commits;
    int bugfix_commits;
    int feature_commits;
} Author;

int find_author(Author *authors, int count, const char *email) {
    for (int i = 0; i < count; i++) {
        if (strcmp(authors[i].email, email) == 0)
            return i;
    }
    return -1;
}

int is_bugfix(const char *msg) {
    return strstr(msg, "fix") || strstr(msg, "bug");
}

int is_feature(const char *msg) {
    return strstr(msg, "feat") || strstr(msg, "feature");
}

int main() {
    FILE *fp = popen("git log --pretty=format:\"%ae|%s\"", "r");
    if (!fp) {
        perror("git log failed");
        return 1;
    }

    Author authors[MAX_AUTHORS];
    int author_count = 0;
    int total_commits = 0;

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;

        char *email = strtok(line, "|");
        char *msg = strtok(NULL, "|");

        if (!email || !msg)
            continue;

        int idx = find_author(authors, author_count, email);
        if (idx < 0) {
            idx = author_count++;
            strcpy(authors[idx].email, email);
            authors[idx].commits = 0;
            authors[idx].bugfix_commits = 0;
            authors[idx].feature_commits = 0;
        }

        authors[idx].commits++;
        total_commits++;

        if (is_bugfix(msg))
            authors[idx].bugfix_commits++;
        if (is_feature(msg))
            authors[idx].feature_commits++;
    }

    pclose(fp);

    int one_time = 0;
    int repeat = 0;
    int one_time_bugfixers = 0;
    int one_time_features = 0;

    for (int i = 0; i < author_count; i++) {
        if (authors[i].commits == 1) {
            one_time++;

            if (authors[i].bugfix_commits == 1)
                one_time_bugfixers++;

            if (authors[i].feature_commits == 1)
                one_time_features++;
        } else {
            repeat++;
        }
    }

    printf("Total commits: %d\n", total_commits);
    printf("Total contributors: %d\n\n", author_count);

    printf("One-time contributors: %d (%.2f%%)\n",
           one_time, 100.0 * one_time / author_count);

    printf("Repeat committers: %d (%.2f%%)\n",
           repeat, 100.0 * repeat / author_count);

    printf("One-time bugfixers: %d (%.2f%%)\n",
           one_time_bugfixers,
           100.0 * one_time_bugfixers / author_count);

    printf("One-time feature contributors: %d (%.2f%%)\n\n",
           one_time_features,
           100.0 * one_time_features / author_count);

    printf("Contribution by author:\n");
    for (int i = 0; i < author_count; i++) {
        double percent = 100.0 * authors[i].commits / total_commits;
        printf("  %s: %d commits (%.2f%%)\n",
               authors[i].email,
               authors[i].commits,
               percent);
    }

    return 0;
}
