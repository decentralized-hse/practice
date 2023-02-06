#include <sodium.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

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
    stringstream keyb;
    keyb << keyfile.rdbuf();

    ifstream signfile(string(argv[1]) + ".sign");
    if (!signfile) {
        cout << "Signature key file not found.\n";
        return 1;
    }
    stringstream signb;
    signb << signfile.rdbuf();

    int verified = crypto_sign_ed25519_verify_detached((const unsigned char *)(signb.str().c_str()),
            (const unsigned char *)(mb.str().c_str()), mb.str().size(),
            (const unsigned char *)(keyb.str().c_str()));
    if (verified == 0) {
        cout << "Message signature is correct!\n";
        return 0;
    } else {
        cout << "Message signature is bad!\n";
        return 1;
    }
}
