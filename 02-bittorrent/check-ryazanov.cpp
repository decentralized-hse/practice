#include <sodium.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

char h2c(char h) {
    h = tolower(h);
    if ('0' <= h && h <= '9') {
        return h - '0';
    }
    if ('a' <= h && h <= 'f') {
        return 10 + h - 'a';
    }
    throw invalid_argument("Bad character for hex");
}

string hex2b(const string& hex_str) {
    if (hex_str.size() % 2 != 0) {
        throw invalid_argument("Bad size for hex string");
    }
    string b_string;
    b_string.reserve(hex_str.size()/2);
    for (size_t i = 0; i < hex_str.size(); i += 2) {
        b_string.push_back(h2c(hex_str[i])*16 + h2c(hex_str[i+1]));
    }
    return b_string;
}

int main(int argc, char *argv[]) {
    if (sodium_init() < 0) {
        cout << "Sodium couldn't be initialized.\n";
        return 1;
    }

    ifstream hashfile(string(argv[1]) + ".root");
    if (!hashfile) {
        cout << "Hash file not found.\n";
        return 1;
    }
    stringstream mb;
    mb << hashfile.rdbuf();

    ifstream keyfile(string(argv[1]) + ".pub");
    if (!keyfile) {
        cout << "Public key file not found.\n";
        return 1;
    }
    string key_hex;
    keyfile >> key_hex;
    string keyb = hex2b(key_hex);

    ifstream signfile(string(argv[1]) + ".sign");
    if (!signfile) {
        cout << "Signature key file not found.\n";
        return 1;
    }
    string sign_hex;
    signfile >> sign_hex;
    string signb = hex2b(sign_hex);

    int verified = crypto_sign_ed25519_verify_detached((const unsigned char *)(signb.c_str()),
            (const unsigned char *)(mb.str().c_str()), mb.str().size(),
            (const unsigned char *)(keyb.c_str()));
    if (verified == 0) {
        cout << "Message signature is correct!\n";
        return 0;
    } else {
        cout << "Message signature is bad!\n";
        return 1;
    }
}
