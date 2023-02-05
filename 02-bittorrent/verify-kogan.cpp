// g++ -overify verify-kogan.cpp -lsodium && ./verify datafile 5

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sodium.h>

class Hash {
public:
    const static int HASH_LENGTH = 32;
    const static int HEX_LENGTH = 64 + 1;

    unsigned char hash[HASH_LENGTH];

    Hash() {
        for (int i = 0; i < HASH_LENGTH; ++i) {
            hash[i] = 0;
        }
    }

    friend std::istream &operator>> (std::istream &inp, Hash &h) {
        char hex[HEX_LENGTH + 1];
        inp.read(hex, HEX_LENGTH);
        sodium_hex2bin(h.hash, HASH_LENGTH, hex, HEX_LENGTH + 1, NULL, NULL, NULL);
        return inp;
    }

    friend std::ostream &operator<< (std::ostream &out, const Hash &h) {
        char hex[HEX_LENGTH];
        sodium_bin2hex(hex, HEX_LENGTH, h.hash, HASH_LENGTH);
        out.write(hex, HEX_LENGTH);
        return out;
    }

    bool operator== (const Hash &other) const {
        for (int i = 0; i < HASH_LENGTH; ++i) {
            if (hash[i] != other.hash[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!= (const Hash &other) const {
        return !(*this == other);
    }
};

Hash calculate_hash_from_children(Hash left, Hash right) {
    char left_hex[Hash::HEX_LENGTH];
    sodium_bin2hex(left_hex, Hash::HEX_LENGTH, left.hash, Hash::HASH_LENGTH);
    left_hex[Hash::HEX_LENGTH - 1] = '\n';
    char right_hex[Hash::HEX_LENGTH];
    sodium_bin2hex(right_hex, Hash::HEX_LENGTH, right.hash, Hash::HASH_LENGTH);
    right_hex[Hash::HEX_LENGTH - 1] = '\n';
    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, (unsigned char*)&left_hex, Hash::HEX_LENGTH);
    crypto_hash_sha256_update(&state, (unsigned char*)&right_hex, Hash::HEX_LENGTH);
    Hash res;
    crypto_hash_sha256_final(&state, (unsigned char*)&res.hash);
    std::cout << "hash(" << left << ", " << right << ") = " << res << "\n";
    return res;
}

int get_block_ndx(const std::string &chunk_fname) {
    int block_ndx;
    std::stringstream ss;
    ss << chunk_fname;
    ss >> block_ndx;
    return block_ndx;
}

Hash read_single_hash(const std::string &fname) {
    std::cout << "Reading " << fname << "\n";
    Hash res;
    std::fstream fin(fname, std::ios_base::in);
    fin >> res;
    return res;
}

std::vector<Hash> read_multiple_hashes(const std::string &fname) {
    std::vector<Hash> res;
    // std::ifstream fin(fname);
    std::ifstream fin(fname);
    Hash cur_hash;
    while (fin >> cur_hash) {
        res.push_back(cur_hash);
    }
    std::cout << "Read " << res.size()<<" hashes from "<< fname << "\n";
    return res;
}

Hash calculate_root_hash(const std::string &peaks_fname) {
    const int SIZE = 65 * 32;
    char peaks[SIZE];
    std::fstream fin(peaks_fname, std::ios_base::in);
    fin.read(peaks, SIZE);
    Hash res;
    crypto_hash_sha256((unsigned char*)&res.hash, (unsigned char*)peaks, SIZE);
    return res;
}

bool verify_peaks(const std::string &peaks_fname, Hash root_hash) {
    return calculate_root_hash(peaks_fname) == root_hash;
}

int calculate_local_block_ndx(const std::vector<Hash> &peaks, int block_ndx) {
    long long res = 0;
    for (int i = peaks.size() - 1; i >= 0; --i) {
        if (peaks[i] != Hash()) {
            res += 1 << i;
            if (res >= block_ndx)
                return block_ndx + (1 << i) - res;
        }
    }
    return -1;
}

int calculate_chunk_peak_level(const std::vector<Hash> &peaks, int block_ndx) {
    int cur_file_size = 0;
    for (int i = peaks.size() - 1; i >= 0; --i) {
        if (peaks[i] != Hash()) {
            cur_file_size += 1 << i;
            if (cur_file_size >= block_ndx) {
                return i;
            }
        }
    }
    return -1;
}

Hash calculate_chunk_hash(const std::string &chunk_fname) {
    std::cout << "Reading a chunk from " << chunk_fname << "\n";
    const int SIZE = 1024;
    char chunk[SIZE];
    std::fstream fin(chunk_fname, std::ios_base::in);
    fin.read(chunk, SIZE);
    Hash res;
    crypto_hash_sha256((unsigned char*)&res.hash, (unsigned char*)chunk, SIZE);
    std::cout << "De-facto chunk hash is "<<res<<"\n";
    return res;
}

Hash calculate_proof_hash(const std::vector<Hash> uncles, Hash chunk_hash, int local_block_ndx, int chunk_peak_level) {
    int level_index = local_block_ndx;
    Hash hash = chunk_hash;
    for (int level = 0; level < chunk_peak_level; ++level) {
        if (level_index & 1) {
            hash = calculate_hash_from_children(uncles[level], hash);
        } else {
            hash = calculate_hash_from_children(hash, uncles[level]);
        }
        std::cout << "at level " << level_index << ", (grand)parent hash is " << hash << "\n";
        level_index /= 2;
    }
    return hash;
}

bool verify_proof(const std::vector<Hash> peaks, const std::vector<Hash> uncles, Hash chunk_hash, int local_block_ndx, int chunk_peak_level) {
    return calculate_proof_hash(uncles, chunk_hash, local_block_ndx, chunk_peak_level) == peaks[chunk_peak_level];
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Usage: ./verify-kogan data-file block-number\n";
        return -1;
    }
    if (sodium_init() < 0) {
        return -1;
    }
    const std::string data_file = (std::string)argv[1];
    const int block_ndx = std::stoi(argv[2]);
    const std::string peaks_fname =  data_file+ ".peaks";
    const std::string root_fname = data_file + ".root";
    const std::string proof_fname = data_file + "." + (std::string)argv[2] + ".proof";
    const std::string chunk_fname = data_file + "." + (std::string)argv[2] + ".chunk";
    const std::vector<Hash> peaks = read_multiple_hashes(peaks_fname);
    const Hash root_hash = read_single_hash(root_fname);
    if (!verify_peaks(peaks_fname, root_hash)) {
        std::cout << "Invalid peaks sequence\n";
        return -1;
    }
    //const int block_ndx = block_ndx;
    const int local_block_ndx = calculate_local_block_ndx(peaks, block_ndx);
    if (local_block_ndx == -1) {
        std::cout << local_block_ndx << '\n';
        std::cout << "Invalid chunk index\n";
        return -1;
    }
    std::cout << "verifying block #" << block_ndx << " (within its peak, block #" << local_block_ndx <<")\n";
    const int chunk_peak_level = calculate_chunk_peak_level(peaks, block_ndx);
    const Hash chunk_hash = calculate_chunk_hash(chunk_fname);
    std::vector<Hash> uncles = read_multiple_hashes(proof_fname);
    if (uncles.size() != chunk_peak_level) {
        std::cout << "Invalid uncles amount: " << uncles.size() << " vs " << chunk_peak_level << '\n';
        return -1;
    }
    if (verify_proof(peaks, uncles, chunk_hash, local_block_ndx, chunk_peak_level)) {
        std::cout << "Success!\n";
        return 0;
    }
    std::cout << "Invalid proof hash\n";
    return -1;
}

