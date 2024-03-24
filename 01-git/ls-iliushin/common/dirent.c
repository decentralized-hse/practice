#include "dirent.h"
#include "objutil.h"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#ifdef __STDC_LIB_EXT1__
#endif

static unsigned char last_chr(const unsigned char* str) {
  assert(str != NULL);
  size_t len = strlen(str);
  return str[len - 1];
}

int is_file(const dir_ent_t *ent) {
  assert(ent != NULL);
  return last_chr(ent->name) == ':';
}

int is_dir(const dir_ent_t *ent) {
  assert(ent != NULL);
  return last_chr(ent->name) == '/';
}

int open_dir_obj_by_hash(const unsigned char hash[HASH_LEN + 1],
                         dir_iter_t *dir_iter_out) {
  unsigned char obj_path[HASH_LEN + 2];
  obj_path_by_hash(hash, obj_path);
  return open_dir_obj(obj_path, dir_iter_out);
}

int open_dir_obj(const unsigned char *obj_path, dir_iter_t *dir_iter_out) {
  assert(obj_path != NULL);
  assert(dir_iter_out != NULL);
  FILE *f = fopen(obj_path, "r");
  if (f == NULL) {
    fprintf(stderr, "failed to open dir: %s: %s\n", obj_path, strerror(errno));
    return -1;
  }
  dir_iter_out->f = f;
  dir_iter_out->current.name[0] = '\0';
  dir_iter_out->current.hash[0] = '\0';
  return 0;
}

int read_next_ent(dir_iter_t *dir, dir_ent_t* ent_out) {
  assert(ent_out != NULL);
  if (dir->f == NULL) {
    
  }

  int res = fscanf(dir->f, "%s", ent_out->name);
  if (res == EOF) {
    if (errno != 0) {
      return -2;
    }
    ent_out[0].name[0] = '\0';
    ent_out[0].hash[0] = '\0';
    return 0;
  }
  res = fscanf(dir->f, "%s", ent_out->hash);
  if (res == EOF) {
    return -2;
  }
  return 1;
}

dir_ent_t *next_ent(dir_iter_t *dir) {
  assert(dir != NULL);
  int res = read_next_ent(dir, &dir->current);
  if (res != -2) {
    return &dir->current;
  }
  fprintf(stderr, "failed to iter dir: %s\n", strerror(errno));
  return NULL;
}

dir_ent_t *find_in_dir(dir_iter_t *dir, const unsigned char *name) {
  dir_ent_t *ent = NULL;
  dirent_foreach(dir, ent) {
    // strlen(ent->name) - 1, because the last symbol is a separator
    if (strncmp(name, ent->name, strlen(ent->name) - 1) == 0) {
      return ent;
    }
  }
  return NULL;
}

int go_down(const dir_ent_t *current_ent, dir_iter_t *dir_iter_out) {
  assert(current_ent != NULL);
  assert(dir_iter_out != NULL);
  unsigned char next_dir_obj_path[HASH_LEN + 2];
  if (!is_dir(current_ent)) {
    fprintf(stderr, "%s is not a directory\n", current_ent->name);
    return -1;
  }
  return open_dir_obj_by_hash(current_ent->hash, dir_iter_out);
}

void close_dir(dir_iter_t *dir) {
  assert(dir != NULL);
  fclose(dir->f);
  dir->f = NULL;
  dir->current.name[0] = '\0';
  dir->current.hash[0] = '\0';
}

int reset_dir_iter(dir_iter_t *dir) {
  dir->current.name[0] = '\0';
  dir->current.hash[0] = '\0';
  int res = fseek(dir->f, 0, SEEK_SET);
  if (res != 0) {
    fprintf(stderr, "failed to reset dir iter: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}
