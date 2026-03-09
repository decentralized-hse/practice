#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define INITIAL_CAPACITY  512
#define MAX_LINE          8192   /* handles long commit subjects  */
#define MAX_CMD           4096   /* shell command buffer          */
#define TOP_N             30     /* rows in the contributor table */

typedef struct {
    char *email;
    char *name;
    int   commits;
    int   bugfix_commits;
    int   feature_commits;
    int   merge_commits;
    int   doc_commits;
    int   test_commits;
    int   refactor_commits;
} Author;

typedef struct {
    Author *data;
    int     count;
    int     capacity;
} AuthorTable;

static void table_init(AuthorTable *t) {
    t->capacity = INITIAL_CAPACITY;
    t->count    = 0;
    t->data     = calloc((size_t)t->capacity, sizeof(Author));
    if (!t->data) { perror("calloc"); exit(1); }
}

static void table_free(AuthorTable *t) {
    for (int i = 0; i < t->count; i++) {
        free(t->data[i].email);
        free(t->data[i].name);
    }
    free(t->data);
}

static int table_find(const AuthorTable *t, const char *email) {
    for (int i = 0; i < t->count; i++)
        if (strcmp(t->data[i].email, email) == 0)
            return i;
    return -1;
}

static int table_add(AuthorTable *t, const char *email, const char *name) {
    if (t->count == t->capacity) {
        t->capacity *= 2;
        t->data = realloc(t->data, (size_t)t->capacity * sizeof(Author));
        if (!t->data) { perror("realloc"); exit(1); }
    }
    int idx = t->count++;
    memset(&t->data[idx], 0, sizeof(Author));
    t->data[idx].email = strdup(email);
    t->data[idx].name  = strdup(name ? name : "");
    return idx;
}

static int ihas(const char *hay, const char *needle) {
    size_t nlen = strlen(needle);
    for (; *hay; hay++)
        if (strncasecmp(hay, needle, nlen) == 0) return 1;
    return 0;
}

static int is_merge(const char *m) {
    return ihas(m, "merge branch")   || ihas(m, "merge pull req") ||
           ihas(m, "merge remote")   || ihas(m, "merge tag");
}

static int is_bugfix(const char *m) {
    return ihas(m, "fix")    || ihas(m, "bug")    ||
           ihas(m, "revert") || ihas(m, "regress") ||
           ihas(m, "broken") || ihas(m, "error")   ||
           ihas(m, "crash")  || ihas(m, "oops")    ||
           ihas(m, "patch");
}

static int is_feature(const char *m) {
    return ihas(m, "feat")       || ihas(m, "add ")     ||
           ihas(m, "new ")       || ihas(m, "support ")  ||
           ihas(m, "implement")  || ihas(m, "introduce") ||
           ihas(m, "enable ")    || ihas(m, "initial ");
}

static int is_doc(const char *m) {
    return ihas(m, "doc")     || ihas(m, "readme")  ||
           ihas(m, "comment") || ihas(m, "typo")    ||
           ihas(m, "spelling")|| ihas(m, "grammar");
}

static int is_test(const char *m) {
    return ihas(m, "test")  || ihas(m, "spec")     ||
           ihas(m, "check") || ihas(m, "unittest");
}

static int is_refactor(const char *m) {
    return ihas(m, "refactor") || ihas(m, "clean")   ||
           ihas(m, "tidy")     || ihas(m, "rename")   ||
           ihas(m, "move ")    || ihas(m, "simplif");
}

static int cmp_desc(const void *a, const void *b) {
    return ((const Author *)b)->commits - ((const Author *)a)->commits;
}

static int run_cmd(const char *cmd) {
    fprintf(stderr, "  $ %s\n", cmd);
    fflush(stderr);
    return system(cmd);
}

static int is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void analyse(const char *repodir, const char *ref,
                    int shallow_depth,
                    AuthorTable *authors,
                    int *total_commits, int *total_merges)
{
    char cmd[MAX_CMD];

    if (ref && *ref)
        snprintf(cmd, sizeof(cmd),
            "git -C \"%s\" log \"%s\" "
            "--pretty=format:\"%%an\x1e%%ae\x1f%%s\"",
            repodir, ref);
    else
        snprintf(cmd, sizeof(cmd),
            "git -C \"%s\" log "
            "--pretty=format:\"%%an\x1e%%ae\x1f%%s\"",
            repodir);

    FILE *fp = popen(cmd, "r");
    if (!fp) { perror("popen(git log)"); exit(1); }

    char line[MAX_LINE];
    int  n = 0;

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        char *us = strchr(line, '\x1f');
        if (!us) continue;
        *us = '\0';
        const char *subj = us + 1;

        char *rs = strchr(line, '\x1e');
        const char *name, *email;
        if (rs) { *rs = '\0'; name = line; email = rs + 1; }
        else    { name = "";              email = line;    }

        if (!*email) continue;

        (*total_commits)++;
        n++;

        if (shallow_depth > 0 && n % 5000 == 0) {
            fprintf(stderr, "\r  analysed %d / ~%d commits ...",
                    n, shallow_depth);
            fflush(stderr);
        }

        int idx = table_find(authors, email);
        if (idx < 0) idx = table_add(authors, email, name);

        authors->data[idx].commits++;

        if (is_merge(subj)) {
            authors->data[idx].merge_commits++;
            (*total_merges)++;
            continue;
        }

        if (is_bugfix(subj))    authors->data[idx].bugfix_commits++;
        if (is_feature(subj))   authors->data[idx].feature_commits++;
        if (is_doc(subj))       authors->data[idx].doc_commits++;
        if (is_test(subj))      authors->data[idx].test_commits++;
        if (is_refactor(subj))  authors->data[idx].refactor_commits++;
    }

    pclose(fp);
    if (shallow_depth > 0 && n > 0) {
        fprintf(stderr, "\r  analysed %d commits.            \n", n);
        fflush(stderr);
    }
}

static void print_report(AuthorTable *authors,
                         int total_commits, int total_merges,
                         const char *label)
{
    int ac = authors->count;
    if (ac == 0) return;

    int one_time = 0, repeat = 0;
    int one_time_bugfixers = 0, one_time_features = 0;
    int pure_bugfixers = 0, pure_features = 0, mixed = 0;
    int doc_contributors = 0, test_contributors = 0, refactor_contributors = 0;

    for (int i = 0; i < ac; i++) {
        Author *a = &authors->data[i];

        if (a->commits == 1) {
            one_time++;
            if (a->bugfix_commits  > 0) one_time_bugfixers++;
            if (a->feature_commits > 0) one_time_features++;
        } else {
            repeat++;
        }

        int hb = a->bugfix_commits  > 0;
        int hf = a->feature_commits > 0;
        if (hb && hf) mixed++;
        else if (hb)  pure_bugfixers++;
        else if (hf)  pure_features++;

        if (a->doc_commits      > 0) doc_contributors++;
        if (a->test_commits     > 0) test_contributors++;
        if (a->refactor_commits > 0) refactor_contributors++;
    }

    int real = total_commits - total_merges;

    printf("\n============================================================\n");
    printf("  Git Contributor Stats  |  %s\n", label);
    printf("============================================================\n\n");

    printf("Total commits               : %d\n", total_commits);
    printf("  Merge commits             : %d  (%.1f%%)\n",
           total_merges, 100.0 * total_merges / total_commits);
    printf("  Code / doc commits        : %d  (%.1f%%)\n",
           real, 100.0 * real / total_commits);
    printf("Total unique contributors   : %d\n\n", ac);

    printf("-- By commit frequency ----------------------------------------\n");
    printf("  One-time contributors     : %6d  (%5.1f%% of contributors)\n",
           one_time, 100.0 * one_time / ac);
    printf("  Repeat committers         : %6d  (%5.1f%% of contributors)\n",
           repeat,   100.0 * repeat   / ac);

    printf("\n-- One-time contributor breakdown ----------------------------\n");
    printf("  One-time bugfixers        : %6d  (%5.1f%% of all contributors)\n",
           one_time_bugfixers, 100.0 * one_time_bugfixers / ac);
    printf("  One-time feature devs     : %6d  (%5.1f%% of all contributors)\n",
           one_time_features,  100.0 * one_time_features  / ac);

    printf("\n-- By commit type (all contributors) -------------------------\n");
    printf("  Bugfix-only contributors  : %6d  (%5.1f%%)\n",
           pure_bugfixers,        100.0 * pure_bugfixers        / ac);
    printf("  Feature-only contributors : %6d  (%5.1f%%)\n",
           pure_features,         100.0 * pure_features         / ac);
    printf("  Mixed bug+feature         : %6d  (%5.1f%%)\n",
           mixed,                 100.0 * mixed                 / ac);
    printf("  Docs contributors         : %6d  (%5.1f%%)\n",
           doc_contributors,      100.0 * doc_contributors      / ac);
    printf("  Test contributors         : %6d  (%5.1f%%)\n",
           test_contributors,     100.0 * test_contributors     / ac);
    printf("  Refactor contributors     : %6d  (%5.1f%%)\n",
           refactor_contributors, 100.0 * refactor_contributors / ac);

    qsort(authors->data, ac, sizeof(Author), cmp_desc);

    int show = ac < TOP_N ? ac : TOP_N;

    printf("\n-- Top %d contributors ----------------------------------------\n",
           show);
    printf("  %-28s  %-34s  %7s  %5s  %4s  %4s  %4s  %4s\n",
           "Name", "Email", "Commits", "Share", "Bug", "Feat", "Doc", "Tst");
    printf("  %s\n",
           "-----------------------------------------------------------------------"
           "-----------------------------");

    for (int i = 0; i < show; i++) {
        Author *a = &authors->data[i];
        char dn[29], de[35];
        snprintf(dn, sizeof(dn), "%.28s", a->name);
        snprintf(de, sizeof(de), "%.34s", a->email);
        printf("  %-28s  %-34s  %7d  %4.1f%%  %4d  %4d  %4d  %4d\n",
               dn, de,
               a->commits,
               100.0 * a->commits / total_commits,
               a->bugfix_commits,
               a->feature_commits,
               a->doc_commits,
               a->test_commits);
    }
    if (show < ac)
        printf("  … and %d more contributors\n", ac - show);
    printf("\n");
}

static void usage(const char *prog) {
    fprintf(stderr,
        "\nUsage:\n"
        "  %s                                      # local repo (cwd)\n"
        "  %s <url>                                # remote repo, HEAD\n"
        "  %s <url>  <tag|branch>                  # remote, specific ref\n"
        "  %s <url>  <tag|branch>  <depth>         # shallow (fast!)\n"
        "  %s <local-path>  [ref]                  # explicit local path\n"
        "\nExamples:\n"
        "  %s https://github.com/git/git\n"
        "  %s https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git  v7.0-rc2\n"
        "  %s https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git  v7.0-rc2  5000\n"
        "\nNote: depth=0 means full clone. For the Linux kernel (1M+ commits)\n"
        "      a shallow depth of 5000–50000 is recommended for speed.\n\n",
        prog, prog, prog, prog, prog, prog, prog, prog);
}

int main(int argc, char *argv[])
{
    if (argc >= 2 &&
        (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        usage(argv[0]);
        return 0;
    }

    const char *arg1          = (argc >= 2) ? argv[1] : NULL;
    const char *ref           = (argc >= 3) ? argv[2] : NULL;
    int         shallow_depth = (argc >= 4) ? atoi(argv[3]) : 0;

    int  is_remote = 0;
    char repodir[512];
    char tmpdir[512] = "";
    char cmd[MAX_CMD];
    int  cleanup = 0;

    if (!arg1) {
        /* no argument: use cwd */
        snprintf(repodir, sizeof(repodir), ".");
    } else if (strncmp(arg1, "http://",  7) == 0 ||
               strncmp(arg1, "https://", 8) == 0 ||
               strncmp(arg1, "git://",   6) == 0 ||
               strncmp(arg1, "git@",     4) == 0) {
        is_remote = 1;
        snprintf(tmpdir,   sizeof(tmpdir),   "/tmp/git_stats_%d", (int)getpid());
        snprintf(repodir,  sizeof(repodir),  "%s", tmpdir);
    } else {
        snprintf(repodir, sizeof(repodir), "%s", arg1);
    }

    if (is_remote) {
        fprintf(stderr, "\n[git_stats] Target : %s\n", arg1);
        if (ref)           fprintf(stderr, "[git_stats] Ref    : %s\n", ref);
        if (shallow_depth) fprintf(stderr, "[git_stats] Depth  : %d\n", shallow_depth);
        fprintf(stderr, "[git_stats] Cloning (blobless — metadata only) ...\n\n");

        /*
         * --filter=blob:none  → skip downloading file contents entirely.
         *   We only need commit metadata (author, subject).  The clone is
         *   fast and small even for multi-million-commit repos.
         *
         * --no-checkout       → don't extract a working tree.
         *
         * --single-branch     → only the requested branch/tag history.
         *
         * --depth N           → further truncate to last N commits (optional).
         */

        if (shallow_depth > 0 && ref && *ref) {
            snprintf(cmd, sizeof(cmd),
                "git clone --depth %d --branch \"%s\" "
                "--filter=blob:none --no-checkout --single-branch "
                "\"%s\" \"%s\"",
                shallow_depth, ref, arg1, tmpdir);
        } else if (shallow_depth > 0) {
            snprintf(cmd, sizeof(cmd),
                "git clone --depth %d "
                "--filter=blob:none --no-checkout "
                "\"%s\" \"%s\"",
                shallow_depth, arg1, tmpdir);
        } else if (ref && *ref) {
            snprintf(cmd, sizeof(cmd),
                "git clone --branch \"%s\" "
                "--filter=blob:none --no-checkout --single-branch "
                "\"%s\" \"%s\"",
                ref, arg1, tmpdir);
        } else {
            snprintf(cmd, sizeof(cmd),
                "git clone --filter=blob:none --no-checkout "
                "\"%s\" \"%s\"",
                arg1, tmpdir);
        }

        if (run_cmd(cmd) != 0) {
            fprintf(stderr, "\n[git_stats] ERROR: clone failed.\n");
            return 1;
        }
        cleanup = 1;
        fprintf(stderr, "\n[git_stats] Clone complete. Analysing commits ...\n\n");
    }

    {
        char gitdir[600];
        snprintf(gitdir, sizeof(gitdir), "%s/.git", repodir);
        if (!is_dir(gitdir)) {
            snprintf(gitdir, sizeof(gitdir), "%s/HEAD", repodir);
            struct stat st;
            if (stat(gitdir, &st) != 0) {
                fprintf(stderr,
                    "[git_stats] ERROR: '%s' is not a git repository.\n",
                    repodir);
                return 1;
            }
        }
    }

    AuthorTable authors;
    table_init(&authors);
    int total_commits = 0, total_merges = 0;

    analyse(repodir, ref, shallow_depth,
            &authors, &total_commits, &total_merges);

    if (authors.count == 0 || total_commits == 0) {
        fprintf(stderr,
            "[git_stats] No commits found.\n");
        if (cleanup) {
            snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmpdir);
            if (system(cmd) != 0)
                fprintf(stderr, "[git_stats] ERROR: rm -rf failed\n");
        }
        table_free(&authors);
        return 1;
    }

    char label[600];
    if (is_remote) {
        const char *slash = strrchr(arg1, '/');
        const char *short_url = slash ? slash + 1 : arg1;
        if (ref && *ref)
            snprintf(label, sizeof(label), "%s @ %s", short_url, ref);
        else
            snprintf(label, sizeof(label), "%s", short_url);
        if (shallow_depth > 0) {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "  [depth=%d]", shallow_depth);
            strncat(label, tmp, sizeof(label) - strlen(label) - 1);
        }
    } else {
        if (ref && *ref)
            snprintf(label, sizeof(label), "%s @ %s", repodir, ref);
        else
            snprintf(label, sizeof(label), "%s", repodir);
    }

    print_report(&authors, total_commits, total_merges, label);

    if (cleanup) {
        fprintf(stderr, "[git_stats] Cleaning up temp clone ...\n");
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmpdir);
        if (system(cmd) != 0)
            fprintf(stderr, "[git_stats] ERROR: rm -rf failed\n");
    }

    table_free(&authors);
    return 0;
}
