#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <openssl/sha.h>

#include "objutil.h"

static char dig_to_hex(uint8_t dig) {
  if (dig < 10) {
    return '0' + dig;
  }
  return 'a' + dig - 10;
}

static void hash_to_digest(unsigned char* hash, size_t len) {
  for (size_t i = len; i > 0; --i) {
    uint8_t byte = hash[i - 1];
    hash[i * 2 - 2] = dig_to_hex(byte >> 4);
    hash[i * 2 - 1] = dig_to_hex(byte & 15);
  }
}

void obj_hash(const unsigned char *obj, uint64_t len, char unsigned hash_out[HASH_LEN]) {
  SHA256(obj, len, hash_out);
  hash_to_digest(hash_out, SHA256_DIGEST_LENGTH);
}

void obj_path_by_hash(const unsigned char hash[HASH_LEN], char unsigned path_out[HASH_LEN + 2]) {
  path_out[0] = hash[0];
  path_out[1] = hash[1];
  path_out[2] = '/';
  for (int i = 2; i < HASH_LEN; i++) {
    path_out[i + 1] = hash[i];
  }
  path_out[HASH_LEN + 1] = '\0';
}

int write_object(const unsigned char *obj, uint64_t len, unsigned char hash_out[HASH_LEN]) {
  char hash[HASH_LEN];
  obj_hash(obj, len, hash);
  char obj_path[HASH_LEN + 2];
  obj_path_by_hash(hash, obj_path);
  char obj_dir[3];
  obj_dir[0] = hash[0];
  obj_dir[1] = hash[1];
  obj_dir[2] = '\0';
  int err = mkdir(obj_dir, S_IRWXU);
  if (err != 0 && err != EEXIST) {
    fprintf(stderr, "failed to write object: %s: %s\n", obj_path, strerror(errno));
    return -1;
  }
  int fd = open(obj_path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    fprintf(stderr, "failed to write object: %s: %s\n", obj_path, strerror(errno));
    return -1;
  }
  ssize_t n_written = write(fd, obj, len);
  if (n_written != len) {
    fprintf(stderr, "failed to write object: %s: %s\n", obj_path, strerror(errno));
    close(fd);
    remove(obj_path);
    return -1;
  }
  close(fd);
  return 0;
}