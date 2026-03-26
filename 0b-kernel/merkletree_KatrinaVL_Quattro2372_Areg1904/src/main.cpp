#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <memory>
#include <map>
#include <set>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

namespace fs = std::filesystem;

const std::string MANIFEST_FILENAME = "SHA256SUMS";
const std::string SIG_FILENAME      = "SHA256SUMS.sign";

class Crypto {
public:
    using ScopedCTX  = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    using ScopedPKEY = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
    using ScopedBIO  = std::unique_ptr<BIO, decltype(&BIO_free)>;

    static std::string sha256_file_hex(const fs::path& path);
    static std::string sha256_string_hex(const std::string& input);
    static void sign_file(const fs::path& dataPath, const fs::path& sigPath, const fs::path& privKeyPath);
    static bool verify_file(const fs::path& dataPath, const fs::path& sigPath, const fs::path& pubKeyPath);

private:
    static void handle_errors(const std::string& context);
    static std::string get_openssl_error_string();
    static ScopedPKEY load_key(const fs::path& path, bool isPrivate);
    static std::vector<unsigned char> get_raw_file_hash(const fs::path& path);
};

struct FileRecord {
    std::string relPath;
    std::string hash;
    bool operator<(const FileRecord& other) const;
};

class MerkleTree {
public:
    explicit MerkleTree(fs::path dir);
    void build();
    void save_manifest() const;
    std::string get_root_hash() const;
    const std::vector<FileRecord>& get_records() const;
    static std::string load_manifest(const fs::path& dir);

private:
    fs::path rootDir;
    std::vector<FileRecord> records;
    std::string rootHash;
};

void run_generation_mode(const fs::path& dir, const fs::path& key);
void run_verification_mode(const fs::path& dir, const fs::path& key);

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage:\n"
                  << "  Generate: " << argv[0] << " --sign   <dir> <private_key.pem>\n"
                  << "  Verify:   " << argv[0] << " --check  <dir> <public_key.pem>\n";
        return 1;
    }

    OpenSSL_add_all_algorithms();

    std::string mode   = argv[1];
    fs::path targetDir = argv[2];
    fs::path keyPath   = argv[3];

    std::cout << "[*] Target directory : " << fs::absolute(targetDir) << "\n";
    std::cout << "[*] Key file         : " << fs::absolute(keyPath) << "\n";

    try {
        if (mode == "--sign") {
            run_generation_mode(targetDir, keyPath);
        } else if (mode == "--check") {
            run_verification_mode(targetDir, keyPath);
        } else {
            std::cerr << "[ERROR] Unknown mode: " << mode << "\n";
            std::cerr << "        Expected --sign or --check\n";
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] Fatal exception: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

void run_generation_mode(const fs::path& dir, const fs::path& key) {
    std::cout << "[*] Mode: GENERATE & SIGN\n";

    if (!fs::exists(dir)) {
        throw std::runtime_error("Target directory does not exist: " + dir.string());
    }
    if (!fs::is_directory(dir)) {
        throw std::runtime_error("Target path is not a directory: " + dir.string());
    }
    if (!fs::exists(key)) {
        throw std::runtime_error("Private key file not found: " + key.string());
    }

    MerkleTree tree(dir);
    std::cout << "    Scanning directory: " << dir << "...\n";
    tree.build();

    const auto& records = tree.get_records();
    std::cout << "    Files indexed: " << records.size() << "\n";
    for (const auto& rec : records) {
        std::cout << "        " << rec.hash << "  " << rec.relPath << "\n";
    }

    tree.save_manifest();
    std::cout << "    Manifest written : " << (dir / MANIFEST_FILENAME) << "\n";
    std::cout << "    Merkle Root      : " << tree.get_root_hash() << "\n";

    std::cout << "    Signing manifest...\n";
    Crypto::sign_file(dir / MANIFEST_FILENAME, dir / SIG_FILENAME, key);
    std::cout << "    Signature written: " << (dir / SIG_FILENAME) << "\n";

    std::cout << "[+] Protected. Ready for transfer.\n";
}

void run_verification_mode(const fs::path& dir, const fs::path& key) {
    std::cout << "[*] Mode: VERIFY INTEGRITY\n";

    if (!fs::exists(dir)) {
        throw std::runtime_error("Target directory does not exist: " + dir.string());
    }

    fs::path manifestPath = dir / MANIFEST_FILENAME;
    fs::path sigPath      = dir / SIG_FILENAME;

    if (!fs::exists(manifestPath)) {
        throw std::runtime_error("Manifest file not found: " + manifestPath.string());
    }
    if (!fs::exists(sigPath)) {
        throw std::runtime_error("Signature file not found: " + sigPath.string()
            + "\n        Was the directory signed with --sign first?");
    }
    if (!fs::exists(key)) {
        throw std::runtime_error("Public key file not found: " + key.string());
    }

    std::cout << "    Manifest path : " << manifestPath << "\n";
    std::cout << "    Signature path: " << sigPath << "\n";

    std::cout << "    Checking digital signature...\n";
    if (!Crypto::verify_file(manifestPath, sigPath, key)) {
        std::cerr << "[!] Signature verification FAILED.\n";
        std::cerr << "    Possible reasons:\n";
        std::cerr << "      - Wrong public key (does not match the private key used for signing)\n";
        std::cerr << "      - Manifest file was modified after signing\n";
        std::cerr << "      - Signature file was corrupted or replaced\n";
        std::cerr << "    Manifest : " << manifestPath << " ("
                  << fs::file_size(manifestPath) << " bytes)\n";
        std::cerr << "    Signature: " << sigPath << " ("
                  << fs::file_size(sigPath) << " bytes)\n";
        throw std::runtime_error("CRITICAL: Signature is INVALID. The manifest has been tampered with!");
    }
    std::cout << "    Signature OK. Manifest is trusted.\n";

    std::string expectedRoot = MerkleTree::load_manifest(dir);
    std::cout << "    Expected Merkle Root: " << expectedRoot << "\n";

    std::cout << "    Re-scanning filesystem...\n";
    MerkleTree currentTree(dir);
    currentTree.build();

    const auto& currentRecords = currentTree.get_records();
    std::cout << "    Files found on disk: " << currentRecords.size() << "\n";

    std::string actualRoot = currentTree.get_root_hash();
    std::cout << "    Actual Merkle Root  : " << actualRoot << "\n";

    if (actualRoot == expectedRoot) {
        std::cout << "[+] SUCCESS: Filesystem is intact. All "
                  << currentRecords.size() << " file(s) match.\n";
    } else {
        std::cerr << "[!] FAILURE: Root hash mismatch.\n";
        std::cerr << "    Expected : " << expectedRoot << "\n";
        std::cerr << "    Actual   : " << actualRoot << "\n";
        std::cerr << "\n    Detailed file scan:\n";
        for (const auto& rec : currentRecords) {
            std::cerr << "        " << rec.hash << "  " << rec.relPath << "\n";
        }
        exit(1);
    }
}

bool FileRecord::operator<(const FileRecord& other) const {
    return relPath < other.relPath;
}

MerkleTree::MerkleTree(fs::path dir) : rootDir(std::move(dir)) {}

void MerkleTree::build() {
    records.clear();
    std::string hashAccumulator;

    if (!fs::exists(rootDir)) {
        throw std::runtime_error("Directory not found: " + rootDir.string());
    }

    for (const auto& entry : fs::recursive_directory_iterator(rootDir)) {
        if (!entry.is_regular_file()) continue;
        std::string fname = entry.path().filename().string();
        if (fname == MANIFEST_FILENAME || fname == SIG_FILENAME) continue;

        std::string rel = fs::relative(entry.path(), rootDir).string();
        std::replace(rel.begin(), rel.end(), '\\', '/');

        try {
            std::string hash = Crypto::sha256_file_hex(entry.path());
            records.push_back({rel, hash});
        } catch (const std::exception& ex) {
            std::cerr << "[WARN] Cannot hash file, skipping: " << entry.path()
                      << "\n       Reason: " << ex.what() << "\n";
        }
    }

    std::sort(records.begin(), records.end());
    for (const auto& rec : records) hashAccumulator += rec.hash + rec.relPath;
    rootHash = Crypto::sha256_string_hex(hashAccumulator);
}

void MerkleTree::save_manifest() const {
    fs::path outPath = rootDir / MANIFEST_FILENAME;
    std::ofstream out(outPath);
    if (!out) {
        throw std::runtime_error("Cannot write manifest file: " + outPath.string()
            + "\n    Check directory permissions.");
    }
    out << rootHash << "\n";
    if (!out) {
        throw std::runtime_error("Write error while saving manifest: " + outPath.string());
    }
}

std::string MerkleTree::get_root_hash() const { return rootHash; }
const std::vector<FileRecord>& MerkleTree::get_records() const { return records; }

std::string MerkleTree::load_manifest(const fs::path& dir) {
    fs::path manifestPath = dir / MANIFEST_FILENAME;
    std::ifstream in(manifestPath);
    if (!in) {
        throw std::runtime_error("Cannot open manifest for reading: " + manifestPath.string());
    }
    std::string root;
    std::getline(in, root);
    if (root.empty()) {
        throw std::runtime_error("Manifest is empty or malformed: " + manifestPath.string());
    }
    return root;
}

std::string Crypto::get_openssl_error_string() {
    char buf[256];
    unsigned long errCode = ERR_get_error();
    if (errCode == 0) return "(no OpenSSL error queued)";
    ERR_error_string_n(errCode, buf, sizeof(buf));
    return std::string(buf);
}

void Crypto::handle_errors(const std::string& context) {
    throw std::runtime_error(context + " | OpenSSL: " + get_openssl_error_string());
}

Crypto::ScopedPKEY Crypto::load_key(const fs::path& path, bool isPrivate) {
    if (!fs::exists(path)) {
        throw std::runtime_error("Key file does not exist: " + path.string());
    }

    ScopedBIO bio(BIO_new_file(path.string().c_str(), "r"), BIO_free);
    if (!bio) {
        throw std::runtime_error("Cannot open key file: " + path.string()
            + " | OpenSSL: " + get_openssl_error_string());
    }

    EVP_PKEY* pkey = isPrivate
        ? PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr)
        : PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);

    if (!pkey) {
        std::string keyType = isPrivate ? "private" : "public";
        throw std::runtime_error("Failed to parse " + keyType + " key from: " + path.string()
            + "\n    Make sure the file is a valid PEM-encoded " + keyType + " key."
            + "\n    OpenSSL: " + get_openssl_error_string());
    }

    return ScopedPKEY(pkey, EVP_PKEY_free);
}

std::vector<unsigned char> Crypto::get_raw_file_hash(const fs::path& path) {
    ScopedCTX ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx) {
        handle_errors("Failed to allocate EVP_MD_CTX for file: " + path.string());
    }
    if (!EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr)) {
        handle_errors("DigestInit failed for file: " + path.string());
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for hashing: " + path.string()
            + "\n    Check file permissions.");
    }

    char buffer[16384];
    std::streamsize totalRead = 0;
    while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
        std::streamsize count = file.gcount();
        totalRead += count;
        if (!EVP_DigestUpdate(ctx.get(), buffer, static_cast<size_t>(count))) {
            handle_errors("DigestUpdate failed at offset ~" + std::to_string(totalRead)
                + " in file: " + path.string());
        }
        if (file.eof()) break;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    if (!EVP_DigestFinal_ex(ctx.get(), hash, &len)) {
        handle_errors("DigestFinal failed for file: " + path.string());
    }

    return std::vector<unsigned char>(hash, hash + len);
}

std::string Crypto::sha256_file_hex(const fs::path& path) {
    auto raw = get_raw_file_hash(path);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto c : raw) ss << std::setw(2) << static_cast<int>(c);
    return ss.str();
}

std::string Crypto::sha256_string_hex(const std::string& input) {
    ScopedCTX ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx) handle_errors("Failed to allocate EVP_MD_CTX for string hashing");

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    if (!EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr)) handle_errors("DigestInit (string)");
    if (!EVP_DigestUpdate(ctx.get(), input.data(), input.size())) handle_errors("DigestUpdate (string)");
    if (!EVP_DigestFinal_ex(ctx.get(), hash, &len)) handle_errors("DigestFinal (string)");

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < len; i++) ss << std::setw(2) << static_cast<int>(hash[i]);
    return ss.str();
}

void Crypto::sign_file(const fs::path& dataPath, const fs::path& sigPath, const fs::path& privKeyPath) {
    std::cout << "        Hashing manifest for signing: " << dataPath << "\n";
    auto dataHash = get_raw_file_hash(dataPath);
    std::cout << "        Loading private key: " << privKeyPath << "\n";
    auto pkey = load_key(privKeyPath, true);

    ScopedCTX ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx) handle_errors("Failed to allocate EVP_MD_CTX for signing");

    if (EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) <= 0) {
        handle_errors("DigestSignInit failed"
            "\n    Ensure the key type supports raw signing (e.g. Ed25519)");
    }

    size_t len = 0;
    if (EVP_DigestSign(ctx.get(), nullptr, &len, dataHash.data(), dataHash.size()) <= 0) {
        handle_errors("DigestSign (length query) failed");
    }

    std::vector<unsigned char> sig(len);
    if (EVP_DigestSign(ctx.get(), sig.data(), &len, dataHash.data(), dataHash.size()) <= 0) {
        handle_errors("DigestSign (final) failed");
    }
    sig.resize(len);

    std::ofstream out(sigPath, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot write signature file: " + sigPath.string()
            + "\n    Check directory permissions.");
    }
    out.write(reinterpret_cast<const char*>(sig.data()), static_cast<std::streamsize>(len));
    if (!out) {
        throw std::runtime_error("Write error while saving signature: " + sigPath.string());
    }
    std::cout << "        Signature size: " << len << " bytes\n";
}

bool Crypto::verify_file(const fs::path& dataPath, const fs::path& sigPath, const fs::path& pubKeyPath) {
    try {
        std::cerr << "        Hashing manifest for verification: " << dataPath << "\n";
        auto dataHash = get_raw_file_hash(dataPath);

        std::ifstream sigIn(sigPath, std::ios::binary | std::ios::ate);
        if (!sigIn) {
            std::cerr << "        [!] Cannot open signature file: " << sigPath << "\n";
            return false;
        }
        std::streamsize sigLen = sigIn.tellg();
        if (sigLen <= 0) {
            std::cerr << "        [!] Signature file is empty: " << sigPath << "\n";
            return false;
        }
        sigIn.seekg(0);
        std::vector<unsigned char> sig(static_cast<size_t>(sigLen));
        sigIn.read(reinterpret_cast<char*>(sig.data()), sigLen);
        std::cerr << "        Signature size read: " << sigLen << " bytes\n";

        std::cerr << "        Loading public key: " << pubKeyPath << "\n";
        auto pkey = load_key(pubKeyPath, false);

        ScopedCTX ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
        if (!ctx) {
            std::cerr << "        [!] Failed to allocate EVP_MD_CTX\n";
            return false;
        }

        if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) <= 0) {
            std::cerr << "        [!] DigestVerifyInit failed | OpenSSL: "
                      << get_openssl_error_string() << "\n";
            return false;
        }

        int res = EVP_DigestVerify(
            ctx.get(),
            sig.data(), static_cast<size_t>(sigLen),
            dataHash.data(), dataHash.size()
        );

        if (res != 1) {
            std::cerr << "        [!] EVP_DigestVerify returned " << res
                      << " | OpenSSL: " << get_openssl_error_string() << "\n";
        }
        return res == 1;

    } catch (const std::exception& ex) {
        std::cerr << "        [!] Exception during verification: " << ex.what() << "\n";
        return false;
    }
}
