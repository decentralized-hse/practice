#include <iostream>
#include <fstream>
#include <sstream>

#include <sodium.h>

int main(int argc, char* argv[]) {
    // initialize and read the file
    if (sodium_init() < 0) {
        std::cout << "Sodium was not initialized, quitting.\n";
        return 1;
    }

    std::ifstream file(std::string(argv[1]) + ".root");
    if (!file) {
        std::cout << "File was not found.\n";
        return 1;
    }

    std::stringstream data;
    data << file.rdbuf();
    file.close();

    // create keys and signature
    unsigned char pk[crypto_sign_ed25519_PUBLICKEYBYTES];
    unsigned char sk[crypto_sign_ed25519_SECRETKEYBYTES];
    crypto_sign_ed25519_keypair(pk, sk);

    unsigned char sig[crypto_sign_ed25519_BYTES];
    crypto_sign_ed25519_detached(sig, NULL,
                                 (const unsigned char *)(data.str().c_str()),
                                 data.str().size(), sk);

    // write them to files

    if (crypto_sign_ed25519_verify_detached(sig,
                                            (const unsigned char *)(data.str().c_str()), 
                                            data.str().size(), pk) != 0) {
        std::cout << "Could not sign the file, quitting.\n";
        return 1;
    } else {
        std::ofstream pub(std::string(argv[1]) + ".pub", std::ios::trunc);
        char hex[65];
        sodium_bin2hex(hex, 66, pk, crypto_sign_ed25519_PUBLICKEYBYTES);
        pub << std::hex << hex << std::endl;
        pub.close();
        if (!pub) {
            std::cout << "Error occured while creating the public key file, quitting.\n";
            return 1;
        }

        std::ofstream sign(std::string(argv[1]) + ".sign", std::ios::trunc);
        char hexs[129];
        sodium_bin2hex(hexs, 130, sig, crypto_sign_ed25519_BYTES);
        sign << std::hex << hexs << std::endl;
        sign.close();
        if (!sign) {
            std::cout << "Error occured while creating the sign file, quitting.\n";
            return 1;
        }
    }
}