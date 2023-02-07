#include <cstddef>
#include <iostream>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <vector>

#include <sodium.h>

const size_t CHUNK_SIZE = 1024;
const size_t HEX_MAX_LEN = crypto_hash_sha256_BYTES * 2 + 1;

struct Hash {
    bool is_empty;
    char value[HEX_MAX_LEN + 1];

    Hash(): is_empty(false) {}
    Hash(bool is_empty_): is_empty(is_empty_) {}

    void FillEnding() {
        value[HEX_MAX_LEN - 1] = '\n';
        value[HEX_MAX_LEN] = '\0';
    }
};

std::vector<std::string> ParseFile(std::string& filename) {
    std::cout << "Reading from file: " << filename << std::endl;

    std::vector<std::string> chunks;
    std::ifstream is(filename, std::ios::binary | std::ios::ate);
    if (is) {
        size_t size = is.tellg();
        if (size % CHUNK_SIZE != 0) {
            throw std::runtime_error("File contains incomplete block");
        }

        std::string str(CHUNK_SIZE, '\0');
        is.seekg(0);
        for (size_t i = 0; i < size / CHUNK_SIZE; ++i) {
            if(is.read(&str[0], CHUNK_SIZE)) {
                chunks.push_back(str);
            } else {
                throw std::runtime_error("Unexpected rrror in chunk reading");
            }
        }
    } else {
        throw std::runtime_error("Cannot open input file");
    }

    return chunks;
}

Hash CalculateLeafHash(const std::string& chunk) {
    unsigned char out[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(out, (unsigned char *)&chunk[0], CHUNK_SIZE);

    Hash hash;
    sodium_bin2hex(hash.value, HEX_MAX_LEN, out, crypto_hash_sha256_BYTES);
    hash.FillEnding();

    std::cout << "Calculate leaf hash: " << hash.value << std::endl;

    return hash;
}

Hash CalculateNodeHash(const Hash& left, const Hash& right) {
    if (left.is_empty || right.is_empty) {
        return Hash(true);
    }

    unsigned char out[crypto_hash_sha256_BYTES];
    crypto_hash_sha256_state state;

    crypto_hash_sha256_init(&state);

    crypto_hash_sha256_update(&state, (unsigned char *)left.value, HEX_MAX_LEN);
    crypto_hash_sha256_update(&state, (unsigned char *)right.value, HEX_MAX_LEN);

    crypto_hash_sha256_final(&state, out);

    Hash hash;
    sodium_bin2hex(hash.value, HEX_MAX_LEN, out, crypto_hash_sha256_BYTES);
    hash.FillEnding();

    std::cout << "Calculate node hash: " << hash.value << std::endl;

    return hash;
}

std::vector<Hash> CalculateHashTree(std::vector<std::string>& chunks) {
    if (chunks.empty()) {
        return std::vector<Hash>();
    } 

    size_t tree_size = chunks.size() * 2 - 1;
    std::vector<Hash> hash_tree(tree_size);

    for (size_t i = 0; i < chunks.size(); ++i) {
        hash_tree[i * 2] = CalculateLeafHash(chunks[i]);
        hash_tree[i * 2 + 1] = Hash(true);
    }

    for (size_t layer_mult = 2; layer_mult < tree_size; layer_mult *= 2) {
        for (size_t layer_offset = layer_mult - 1; layer_offset + layer_mult / 2 < tree_size; layer_offset += layer_mult * 2) {
            size_t left_child = layer_offset - layer_mult / 2;
            size_t right_child = layer_offset + layer_mult / 2;
            hash_tree[layer_offset] = CalculateNodeHash(hash_tree[left_child], hash_tree[right_child]);
        }
    }

    return hash_tree;
}

void DumpHashes(const std::string& filename, const std::vector<Hash>& hash_tree) {
    std::stringstream ss;
    ss << filename << ".hashtree";

    std::cout << "Writing to file: " << ss.str() << std::endl;

    std::ofstream out(ss.str(), std::ios::out);
    if (out) {
        std::string empty(HEX_MAX_LEN - 1, '0');
        for (size_t i = 0; i < hash_tree.size(); ++i) {
            if (hash_tree[i].is_empty) {
                out << empty << '\n';
            } else {
                out << hash_tree[i].value;
            }
        }
    } else {
        throw std::runtime_error("Cannot open output file");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2 || !argv[1]) {
        throw std::runtime_error("Wrong arguments count");
    }
    
    std::string filename = argv[1];
    std::vector<std::string> chunks = ParseFile(filename);
    std::vector<Hash> hash_tree = CalculateHashTree(chunks);
    DumpHashes(filename, hash_tree);

    return 0;
}
