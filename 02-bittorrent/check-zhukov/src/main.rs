use ed25519_dalek::{PublicKey, Signature, Verifier, PUBLIC_KEY_LENGTH, SIGNATURE_LENGTH};
use std::{env, fs};


fn main() {
    let argv: Vec<String> = env::args().collect();
    assert!(argv.len() == 2, "Incorrect number of arguments");

    let hash_path = argv[1].clone() + ".root";
    let pub_key_path = argv[1].clone() + ".pub";
    let sign_path = argv[1].clone() + ".sign";

    println!("[ Reading hash from {} ... ]", hash_path);
    let hash: Vec<u8> = match fs::read(hash_path) {
        Ok(b) => b,
        Err(e) => panic!("Can't read from file: {}", e),
    };

    println!("[ Reading public key from {} ... ]", pub_key_path);
    let public_key_bytes: [u8; PUBLIC_KEY_LENGTH] = match fs::read(pub_key_path) {
        Ok(b) => b.as_slice().try_into().expect("The length doesn't match"),
        Err(e) => panic!("Can't read from file: {}", e),
    };
    let public_key: PublicKey = match PublicKey::from_bytes(&public_key_bytes) {
        Ok(k) => k,
        Err(e) => panic!("Incorrect public key: {}", e),
    };

    println!("[ Reading signature from {} ... ]", sign_path);
    let sign_bytes: [u8; SIGNATURE_LENGTH] = match fs::read(sign_path) {
        Ok(b) => b.as_slice().try_into().expect("The length doesn't match"),
        Err(e) => panic!("Can't read from file: {}", e),
    };
    let sign: Signature = match Signature::try_from(sign_bytes) {
        Ok(s) => s,
        Err(e) => panic!("Incorrect signature: {}", e),
    };

    assert!(
        public_key.verify(hash.as_slice(), &sign).is_ok(),
        "--- Signature is invalid! ---\n"
    );
    println!("--- Signature is valid! ---");
}
