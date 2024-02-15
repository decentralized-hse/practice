#ifndef DIRENT_H
#define DIRENT_H
#include "objutil.h"

#include <stdio.h>

typedef struct dir_ent {
  unsigned char name[MAX_NAME_LEN];
  unsigned char hash[HASH_LEN + 1];
} dir_ent_t;

int is_file(const dir_ent_t *ent);
int is_dir(const dir_ent_t *ent);

typedef struct dir_iter {
  dir_ent_t current;
  FILE *f;
} dir_iter_t;

int open_dir_obj(const unsigned char *obj_path, dir_iter_t *dir_iter_out);
int open_dir_obj_by_hash(const unsigned char hash[HASH_LEN + 1],
                         dir_iter_t *dir_iter_out);
dir_ent_t *next_ent(dir_iter_t *dir);
dir_ent_t *find_in_dir(dir_iter_t *dir, const unsigned char *name);
int go_down(const dir_ent_t *current_ent, dir_iter_t *dir_iter_out);
void close_dir(dir_iter_t *dir);
int reset_dir_iter(dir_iter_t *dir);

#define dirent_foreach(iter, ent)                                              \
  for (ent = next_ent(iter); ent != NULL && ent->name[0] != '\0';              \
       ent = next_ent(iter))

#endif // DIRENT_H
