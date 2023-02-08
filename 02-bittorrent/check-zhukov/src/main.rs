use ed25519_dalek::{PublicKey, Signature, Verifier, PUBLIC_KEY_LENGTH, SIGNATURE_LENGTH};
use std::{env, fs};


fn main() {
    let argv: Vec<String> = env::args().collect();
    assert!(argv.len() == 2, "Incorrect number of arguments");

    let hash_path = argv[1].clone() + ".root";
    let pub_key_path = argv[1].clone() + ".pub";
    let sign_path = argv[1].clone() + ".sign";
    
    println!("[ Reading hash from {} ... ]", hash_path);
    let hash = fs::read(hash_path).expect("Can't read from file");

    println!("[ Reading public key from {} ... ]", pub_key_path);
    let public_key_bytes_hex = fs::read(pub_key_path).expect("Can't read from file");
    let public_key_bytes: [u8; PUBLIC_KEY_LENGTH] = match hex::decode(public_key_bytes_hex) {
        Ok(b) => b.as_slice().try_into().expect("The length doesn't match"),
        Err(e) => panic!("Can't decode hex file: {}", e),
    };
    let public_key = PublicKey::from_bytes(&public_key_bytes).expect("Incorrect public key");

    println!("[ Reading signature from {} ... ]", sign_path);
    let sign_bytes_hex = fs::read(sign_path).expect("Can't read from file");
    let sign_bytes: [u8; SIGNATURE_LENGTH] = match hex::decode(sign_bytes_hex) {
        Ok(b) => b.as_slice().try_into().expect("The length doesn't match"),
        Err(e) => panic!("Can't decode hex file: {}", e),
    };
    let sign = Signature::try_from(sign_bytes).expect("Incorrect signature");

    assert!(
        public_key.verify(hash.as_slice(), &sign).is_ok(),
        "--- Signature is invalid! ---\n"
    );
    println!("--- Signature is valid! ---");
}
