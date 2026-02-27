#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) die("malloc");
    memcpy(p, s, n);
    return p;
}

typedef struct {
    char    *key;
    int64_t  value;
    int      used;
} MapEntry;

typedef struct {
    MapEntry *entries;
    size_t    cap;
    size_t    len;
} HashMap;

static uint64_t hash_str(const char *s) {
    uint64_t h = 1469598103934665603ULL;
    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 1099511628211ULL;
    }
    return h;
}

static void map_init(HashMap *m, size_t cap) {
    m->cap = cap;
    m->len = 0;
    m->entries = calloc(cap, sizeof(MapEntry));
    if (!m->entries) die("calloc");
}

static void map_grow(HashMap *m);

static MapEntry *map_find(HashMap *m, const char *key) {
    if (m->len * 2 >= m->cap)
        map_grow(m);

    uint64_t h = hash_str(key);
    size_t i = h % m->cap;

    for (;;) {
        MapEntry *e = &m->entries[i];
        if (!e->used)
            return e;
        if (strcmp(e->key, key) == 0)
            return e;
        i = (i + 1) % m->cap;
    }
}

static void map_add(HashMap *m, const char *key, int64_t delta) {
    MapEntry *e = map_find(m, key);
    if (!e->used) {
        e->used = 1;
        e->key = xstrdup(key);
        e->value = delta;
        m->len++;
    } else {
        e->value += delta;
    }
}

static void map_set(HashMap *m, const char *key, int64_t value) {
    MapEntry *e = map_find(m, key);
    if (!e->used) {
        e->used = 1;
        e->key = xstrdup(key);
        e->value = value;
        m->len++;
    } else {
        e->value = value;
    }
}

static int map_get(HashMap *m, const char *key, int64_t *out) {
    uint64_t h = hash_str(key);
    size_t i = h % m->cap;

    for (;;) {
        MapEntry *e = &m->entries[i];
        if (!e->used)
            return 0;
        if (strcmp(e->key, key) == 0) {
            *out = e->value;
            return 1;
        }
        i = (i + 1) % m->cap;
    }
}

static void map_grow(HashMap *m) {
    HashMap n;
    map_init(&n, m->cap * 2);

    for (size_t i = 0; i < m->cap; i++) {
        MapEntry *e = &m->entries[i];
        if (e->used)
            map_set(&n, e->key, e->value);
    }

    free(m->entries);
    *m = n;
}

static void map_free(HashMap *m) {
    for (size_t i = 0; i < m->cap; i++) {
        if (m->entries[i].used)
            free(m->entries[i].key);
    }
    free(m->entries);
}

static FILE *git_popen(const char *repo, const char *cmd) {
    char buf[4096];
    snprintf(buf, sizeof(buf), "cd '%s' && %s", repo, cmd);
    FILE *fp = popen(buf, "r");
    if (!fp) die("popen");
    return fp;
}

static void git_pclose(FILE *fp) {
    if (pclose(fp) == -1)
        die("pclose");
}

static void collect_accumulated_loc(const char *repo, HashMap *map) {
    FILE *fp = git_popen(
        repo,
        "git log --numstat --format= --no-renames"
    );

    char line[8192];
    while (fgets(line, sizeof(line), fp)) {
        char *a = strtok(line, "\t");
        char *d = strtok(NULL, "\t");
        char *p = strtok(NULL, "\n");

        if (!a || !d || !p)
            continue;
        if (a[0] == '-' || d[0] == '-')
            continue;

        int added = atoi(a);
        int deleted = atoi(d);
        map_add(map, p, added + deleted);
    }

    git_pclose(fp);
}

static void collect_net_loc(const char *repo, const char *root, HashMap *map) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "git diff --numstat %s HEAD --no-renames", root);

    FILE *fp = git_popen(repo, cmd);

    char line[8192];
    while (fgets(line, sizeof(line), fp)) {
        char *a = strtok(line, "\t");
        char *d = strtok(NULL, "\t");
        char *p = strtok(NULL, "\n");

        if (!a || !d || !p)
            continue;
        if (a[0] == '-' || d[0] == '-')
            continue;

        int added = atoi(a);
        int deleted = atoi(d);
        map_set(map, p, added - deleted);
    }

    git_pclose(fp);
}

static void collect_sizes(const char *repo, const char *ref, HashMap *map) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "git ls-tree -r -l %s", ref);

    FILE *fp = git_popen(repo, cmd);
    char line[8192];

    while (fgets(line, sizeof(line), fp)) {
        char *tab = strchr(line, '\t');
        if (!tab)
            continue;

        long size = 0;
        if (sscanf(line, "%*s %*s %*s %ld", &size) != 1)
            continue;

        char *path = tab + 1;
        path[strcspn(path, "\n")] = 0;

        map_set(map, path, size);
    }

    git_pclose(fp);
}

typedef struct {
    char   *path;
    int64_t accum_loc;
    int64_t net_loc;
    int64_t size_first;
    int64_t size_last;
    int64_t bin_delta;
} FileStat;

static int cmp_accum_desc(const void *a, const void *b) {
    const FileStat *x = a;
    const FileStat *y = b;
    return (y->accum_loc > x->accum_loc) - (y->accum_loc < x->accum_loc);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <git-repo>\n", argv[0]);
        return 1;
    }

    const char *repo = argv[1];

    FILE *fp = git_popen(repo, "git rev-list --max-parents=0 HEAD");
    char root[64];
    if (!fgets(root, sizeof(root), fp))
        die("cannot get root commit");
    root[strcspn(root, "\n")] = 0;
    git_pclose(fp);

    HashMap accum, net, size0, size1;
    map_init(&accum, 1 << 15);
    map_init(&net,   1 << 15);
    map_init(&size0, 1 << 15);
    map_init(&size1, 1 << 15);

    collect_accumulated_loc(repo, &accum);
    collect_net_loc(repo, root, &net);
    collect_sizes(repo, root, &size0);
    collect_sizes(repo, "HEAD", &size1);

    FileStat *stats = calloc(accum.len, sizeof(FileStat));
    if (!stats) die("calloc");

    size_t n = 0;
    for (size_t i = 0; i < accum.cap; i++) {
        MapEntry *e = &accum.entries[i];
        if (!e->used)
            continue;

        FileStat *s = &stats[n++];
        s->path = e->key;
        s->accum_loc = e->value;

        map_get(&net,   s->path, &s->net_loc);
        map_get(&size0, s->path, &s->size_first);
        map_get(&size1, s->path, &s->size_last);

        s->bin_delta = s->size_last - s->size_first;
    }

    qsort(stats, n, sizeof(FileStat), cmp_accum_desc);

    printf("%-60s %12s %12s %12s\n",
           "file", "accum_loc", "net_loc", "bin_delta");

    for (size_t i = 0; i < n; i++) {
        FileStat *s = &stats[i];
        printf("%-60s %12lld %12lld %12lld\n",
               s->path,
               (long long)s->accum_loc,
               (long long)s->net_loc,
               (long long)s->bin_delta);
    }

    free(stats);
    map_free(&accum);
    map_free(&net);
    map_free(&size0);
    map_free(&size1);

    return 0;
}
