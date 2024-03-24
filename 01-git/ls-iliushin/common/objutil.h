#ifndef OBJUTIL_H
#define OBJUTIL_H

#include <openssl/sha.h>
#include <stdint.h>

#define HASH_LEN (SHA256_DIGEST_LENGTH * 2)
#define MAX_NAME_LEN 512

void obj_hash(const unsigned char *obj, uint64_t len,
              unsigned char hash_out[HASH_LEN]);
void obj_path_by_hash(const unsigned char hash[HASH_LEN],
                      unsigned char path_out[HASH_LEN + 2]);
int write_object(const unsigned char *obj, uint64_t len,
                 unsigned char hash_out[HASH_LEN]);

#endif // OBJUTIL_H
