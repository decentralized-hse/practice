#pragma once

#include <stdlib.h>
#include <openssl/sha.h>

#include "utils.h"

static void to_string(unsigned char hash[SHA256_DIGEST_LENGTH], char hashStr[65]) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hashStr + (i * 2), "%02x", hash[i]);
    }
    hashStr[64] = 0;
}

char* sha256(char *path, char hashStr[65]) {
    FILE *file = fopen(path, "rb");
    require((file), "failed to open file");

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char buffer[bufSize];
    int bytesRead = 0;
    while((bytesRead = fread(buffer, 1, bufSize, file)))
    {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    to_string(hash, hashStr);
    fclose(file);
    return hashStr;
}
