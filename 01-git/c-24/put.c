#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dirent.h"
#include "objutil.h"

#define MAX_PATH_LEN 2048

int parse_out_component_name(const unsigned char* path, unsigned char component_out[MAX_NAME_LEN], uint32_t* len_out) {
  for (uint32_t i = 0; i + 1 < MAX_NAME_LEN;i++) {
    component_out[i] = path[i];
    if (path[i] == '/') {
      component_out[i + 1] = '\0';
      *len_out = i;
      return 1;
    }
    if (path[i] == '\0') {
      component_out[i] = ':';
      component_out[i + 1] = '\0';
      *len_out = i;
      return 0;
    }
  }
  return -1;
}

int put(dir_iter_t* dir, const unsigned char* path, const unsigned char* obj_data, long obj_size, uint32_t idx, unsigned char hash_out[HASH_LEN + 1]) {
  while (path[idx] == '/') {
    ++idx;
    if (idx == MAX_PATH_LEN) {
      fprintf(stderr, "path invalid or too long(max allowed size %d)", MAX_PATH_LEN);
      return -1;
    }
  }
  unsigned char component_name[MAX_NAME_LEN];
  uint32_t component_len = 0;
  int is_final = parse_out_component_name(path + idx, component_name, &component_len);
  if (is_final == -1) {
    fprintf(stderr, "path component too long(max allowed size %d)", MAX_NAME_LEN);
    return -1;
  }

  dir_ent_t* ent = find_in_dir(dir, component_name);
  if (ent != NULL && is_final == 1) {
    fprintf(stderr, "%s already exists", component_name);
    return -1;
  }

  if (ent == NULL && is_final == 0) {
    unsigned char buf[MAX_PATH_LEN];
    memcpy(buf, path, idx + component_len);
    buf[idx + component_len + 1] = '\0';
    fprintf(stderr, "%s directory does not exist", buf);
    return -1;
  }

  unsigned char subobj_hash[HASH_LEN + 1];
  subobj_hash[HASH_LEN] = '\0';
  if (is_final == 0) {
    dir_iter_t subdir_iter;
    int res = go_down(ent, &subdir_iter);
    if (res != 0) {
      return -1;
    }
    res = put(&subdir_iter, path, obj_data, obj_size, idx + component_len + 1, subobj_hash);
    close_dir(&subdir_iter);
    if (res != 0) {
      return -1;
    }
  } else {
    int res = write_object(obj_data, obj_size, subobj_hash);
    if (res != 0) {
      return -1;
    }
  }

  reset_dir_iter(dir);
  uint32_t n_dir_entries = 0;
  size_t dir_obj_len = 1; /* 1 for final \n*/
  dirent_foreach(dir, ent) {
    ++n_dir_entries;
    dir_obj_len += strlen(ent->name) + HASH_LEN + 2;
  }
  n_dir_entries += is_final;
  dir_ent_t* materialized_dir_view = malloc(n_dir_entries * sizeof(dir_ent_t));

  reset_dir_iter(dir);
  uint32_t ent_idx = 0;
  int need_to_insert = is_final;
  dirent_foreach(dir, ent) {
    int cmp_res = strcmp(ent->name, component_name);
    materialized_dir_view[ent_idx] = *ent;
    if (cmp_res == 0 && is_final == 0) {
      memcpy(materialized_dir_view[ent_idx].hash, subobj_hash, sizeof(subobj_hash));
    } else if (cmp_res > 0 && need_to_insert) {
      materialized_dir_view[ent_idx + 1] = materialized_dir_view[ent_idx];
      memcpy(materialized_dir_view[ent_idx].hash, subobj_hash, sizeof(subobj_hash));
      memcpy(materialized_dir_view[ent_idx].name, component_name, sizeof(component_name));
      dir_obj_len += strlen(component_name) + HASH_LEN + 2;
      need_to_insert = 0;
      ++ent_idx;
    }
    ++ent_idx;
  }
  unsigned char* buf = malloc(dir_obj_len + 1); /* +1 for final \0 */
  for (size_t i = 0, offset = 0; i < n_dir_entries; i++) {
    offset += sprintf(buf + offset, "%s\t%s\n", materialized_dir_view[i].name, materialized_dir_view[i].hash);
  }
  free(materialized_dir_view);
  buf[dir_obj_len - 2] = '\n';
  buf[dir_obj_len - 1] = '\0';
  int res = write_object(buf, dir_obj_len, hash_out);
  free(buf);
  return res;
}

int check_args(int argc, char** argv) {
  if (argc != 3) {
    return -1;
  }
  if (strlen(argv[2]) != HASH_LEN) {
    return -1;
  }
  if (strlen(argv[1]) > MAX_PATH_LEN) {
    return -2;
  }
  return 0;
}

unsigned char* read_all(FILE* stream, size_t* len_out) {
  size_t size = 4096;
  size_t offset = 0;
  unsigned char* buf = malloc(size);
  while(1) {
    size_t n_read = fread(buf + offset, 1, size - offset, stream);
    offset += n_read;
    if (feof(stream)) {
      break;
    }
    if (ferror(stream)) {
      free(buf);
      return NULL;
    }

    assert(size == offset);
    size *= 2;
    unsigned char* n_buf = realloc(buf, size);
    if (n_buf == NULL) {
      free(buf);
      return NULL;
    }
    buf = n_buf;
  }
  *len_out = offset;
  return buf;
}

int main(int argc, char** argv) {
  int res = check_args(argc, argv);
  if (res == -1) {
    printf("Invalid arguments. Expected: put <path> <256bit-hash>\n");
    return 1;
  }
  if (res == -2) {
    printf("Invalid arguments. Path too long (max allowed len: %d)", MAX_PATH_LEN);
    return 1;
  }
  dir_iter_t root_iter;
  res = open_dir_obj_by_hash(argv[2], &root_iter);
  if (res != 0) {
    return 1;
  }

  size_t obj_len = 0;
  char* obj_data = read_all(stdin, &obj_len);
  if (obj_data == NULL) {
    close_dir(&root_iter);
    printf("failed to read stdin");
    return 1;
  }
  unsigned char hash_out[HASH_LEN + 1];
  res = put(&root_iter, argv[1], obj_data, obj_len, 0, hash_out);
  close_dir(&root_iter);
  if (res != 0) {
    return 1;
  }
  return 0;
}