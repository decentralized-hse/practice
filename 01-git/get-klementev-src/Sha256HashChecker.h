#pragma once

#include <string>
#include <openssl/evp.h>
#include <ios>
#include <iomanip>

class Sha256HashChecker {
private:
    static bool compute_hash(const std::string &unhashed, std::string &hashed) {
        bool success = false;

        EVP_MD_CTX *context = EVP_MD_CTX_new();

        if (context == nullptr)
            return success;

        if (!EVP_DigestInit_ex(context, EVP_sha256(), nullptr) ||
            !EVP_DigestUpdate(context, unhashed.c_str(), unhashed.length())) {
            EVP_MD_CTX_free(context);
            return success;
        }

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int lengthOfHash = 0;

        if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
            std::stringstream ss;
            for (unsigned int i = 0; i < lengthOfHash; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int) hash[i];
            }

            hashed = ss.str();
            success = true;
        }

        EVP_MD_CTX_free(context);

        return success;
    }

public:
    static bool is_valid(const std::string &buffer, const std::string &hash_expected) {
        std::string actual_hash;
        if (!compute_hash(buffer, actual_hash))
            throw std::runtime_error("Can't calculate hash");

        if (actual_hash != hash_expected)
            return false;

        return true;
    }
};