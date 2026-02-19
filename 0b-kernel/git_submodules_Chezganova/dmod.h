#pragma once

#include <stdint.h>
#include <stddef.h>

#define CID_HEX_LEN    65
#define MAX_NAME       128
#define MAX_PATH       256
#define MAX_CONTENT    4096
#define MAX_ENTRIES    64
#define MAX_NODES      16
#define MAX_SUBMODULES 16
#define REPLICATION    2


// CID
void compute_cid(const uint8_t *data, size_t len, char *out_hex);

// DHT
typedef struct {
    char    cid[CID_HEX_LEN];
    uint8_t content[MAX_CONTENT];
    size_t  content_len;
} DHTEntry;

typedef struct {
    char     id[MAX_NAME];
    DHTEntry entries[MAX_ENTRIES];
    int      entry_count;
} DHTNode;

typedef struct {
    DHTNode *nodes[MAX_NODES];
    int      node_count;
} DHT;

void dht_init(DHT *dht);
void dht_add_node(DHT *dht, DHTNode *node);
void node_init(DHTNode *n, const char *id);
void dht_publish(DHT *dht, const uint8_t *content, size_t len, char *out_cid);
int  dht_fetch(DHT *dht, const char *cid, uint8_t *out_content, size_t *out_len);


// decentralized modules
typedef struct {
    char name[MAX_NAME];
    char path[MAX_PATH];
    char cid[CID_HEX_LEN];
} Submodule;

typedef struct {
    Submodule entries[MAX_SUBMODULES];
    int       count;
} ModulesFile;

int  modules_load(ModulesFile *mf, const char *filepath);
void modules_save(const ModulesFile *mf, const char *filepath);
void modules_print(const ModulesFile *mf);
void modules_upsert(ModulesFile *mf, const char *name,
                    const char *path, const char *cid);

// dmod commands
void dmod_add(DHT *dht, ModulesFile *mf,
              const char *name, const char *path,
              const uint8_t *content, size_t len);

void dmod_update(DHT *dht, const ModulesFile *mf);

void dmod_publish(DHT *dht, ModulesFile *mf,
                  const char *name,
                  const uint8_t *new_content, size_t new_len);

