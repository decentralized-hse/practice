#include "dmod.h"

#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>


void compute_cid(const uint8_t *data, size_t len, char *out_hex) {
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(data, len, digest);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(out_hex + i*2, "%02x", digest[i]);
    out_hex[64] = '\0';
}


void dht_init(DHT *dht) { dht->node_count = 0; }

void node_init(DHTNode *n, const char *id) {
    strncpy(n->id, id, MAX_NAME - 1);
    n->entry_count = 0;
}

void dht_add_node(DHT *dht, DHTNode *node) {
    if (dht->node_count >= MAX_NODES) return;
    dht->nodes[dht->node_count++] = node;
}

static void node_store(DHTNode *n, const char *cid,
                       const uint8_t *content, size_t len) {
    if (n->entry_count >= MAX_ENTRIES) return;
    DHTEntry *e = &n->entries[n->entry_count++];
    strncpy(e->cid, cid, CID_HEX_LEN);
    memcpy(e->content, content, len);
    e->content_len = len;
}

static DHTEntry *node_find(DHTNode *n, const char *cid) {
    for (int i = 0; i < n->entry_count; i++)
        if (strcmp(n->entries[i].cid, cid) == 0)
            return &n->entries[i];
    return NULL;
}


// Узлы выбираются упрощено: берём первые REPLICATION узлов.

void dht_publish(DHT *dht, const uint8_t *content, size_t len, char *out_cid) {
    compute_cid(content, len, out_cid);
    int replicas = dht->node_count < REPLICATION ? dht->node_count : REPLICATION;
    for (int i = 0; i < replicas; i++)
        node_store(dht->nodes[i], out_cid, content, len);
}

int dht_fetch(DHT *dht, const char *cid,
              uint8_t *out_content, size_t *out_len) {
    for (int i = 0; i < dht->node_count; i++) {
        DHTEntry *e = node_find(dht->nodes[i], cid);
        if (!e) continue;

        char actual[CID_HEX_LEN];
        compute_cid(e->content, e->content_len, actual);
        if (strcmp(actual, cid) != 0) {
            fprintf(stderr, "[dht_fetch] integrity error on node '%s'\n",
                    dht->nodes[i]->id);
            return -1;
        }

        memcpy(out_content, e->content, e->content_len);
        *out_len = e->content_len;
        return 0;
    }
    fprintf(stderr, "[dht_fetch] %.16s... not found\n", cid);
    return -1;
}


void modules_upsert(ModulesFile *mf, const char *name,
                    const char *path, const char *cid) {
    for (int i = 0; i < mf->count; i++) {
        if (strcmp(mf->entries[i].name, name) == 0) {
            if (path[0]) strncpy(mf->entries[i].path, path, MAX_PATH - 1);
            strncpy(mf->entries[i].cid, cid, CID_HEX_LEN);
            return;
        }
    }
    Submodule *s = &mf->entries[mf->count++];
    strncpy(s->name, name, MAX_NAME - 1);
    strncpy(s->path, path, MAX_PATH - 1);
    strncpy(s->cid,  cid,  CID_HEX_LEN);
}

void modules_save(const ModulesFile *mf, const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) { perror("modules_save"); return; }
    for (int i = 0; i < mf->count; i++) {
        const Submodule *s = &mf->entries[i];
        fprintf(f, "[submodule \"%s\"]\n\tpath = %s\n\tcid  = %s\n\n",
                s->name, s->path, s->cid);
    }
    fclose(f);
}

int modules_load(ModulesFile *mf, const char *filepath) {
    mf->count = 0;
    FILE *f = fopen(filepath, "r");
    if (!f) return -1;
    char line[512];
    Submodule *cur = NULL;
    while (fgets(line, sizeof(line), f)) {
        char val[MAX_PATH], name[MAX_NAME];
        if (sscanf(line, "[submodule \"%127[^\"]\"]\n", name) == 1) {
            cur = &mf->entries[mf->count++];
            strncpy(cur->name, name, MAX_NAME - 1);
            cur->path[0] = cur->cid[0] = '\0';
        } else if (cur) {
            if      (sscanf(line, " path = %255s", val) == 1)
                strncpy(cur->path, val, MAX_PATH - 1);
            else if (sscanf(line, " cid = %64s",  val) == 1)
                strncpy(cur->cid,  val, CID_HEX_LEN);
        }
    }
    fclose(f);
    return 0;
}

void modules_print(const ModulesFile *mf) {
    printf(".decentralized_modules:\n");
    for (int i = 0; i < mf->count; i++) {
        const Submodule *s = &mf->entries[i];
        printf("  [submodule \"%s\"]\n\tpath = %s\n\tcid  = %s\n",
               s->name, s->path, s->cid);
    }
}


void dmod_add(DHT *dht, ModulesFile *mf,
              const char *name, const char *path,
              const uint8_t *content, size_t len) {
    char cid[CID_HEX_LEN];
    dht_publish(dht, content, len, cid);
    modules_upsert(mf, name, path, cid);
    printf("[dmod add] %s -> cid %.16s...\n", name, cid);
}

void dmod_update(DHT *dht, const ModulesFile *mf) {
    uint8_t buf[MAX_CONTENT];
    size_t  len;
    for (int i = 0; i < mf->count; i++) {
        const Submodule *s = &mf->entries[i];
        printf("[dmod update] fetching '%s'...\n", s->name);
        if (dht_fetch(dht, s->cid, buf, &len) == 0)
            printf("[dmod update] ok -> %s\n", s->path);
        else
            printf("[dmod update] failed: '%s'\n", s->name);
    }
}

void dmod_publish(DHT *dht, ModulesFile *mf,
                  const char *name,
                  const uint8_t *new_content, size_t new_len) {
    char cid[CID_HEX_LEN];
    dht_publish(dht, new_content, new_len, cid);
    modules_upsert(mf, name, "", cid);
    printf("[dmod publish] %s -> new cid %.16s...\n", name, cid);
}