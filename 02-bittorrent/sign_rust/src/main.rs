use ed25519_dalek::{Keypair, Signer};
use rand::rngs::OsRng;
use std::{env, fs};

const INPUT_EXTENSION: &str = ".root";
const OUTPUT_EXTENSION: &str = ".sign";
const PUB_KEY: &str = ".pub";
const SEC_KEY: &str = ".sec";

fn main() {
    // parse arguments
    let args: Vec<String> = env::args().collect();
    assert!(args.len() >= 2);

    // read file
    let input_file_path = &args[1];
    assert!(input_file_path.ends_with(INPUT_EXTENSION));
    let file = fs::read(input_file_path).unwrap();

    // generate key pair and sign file
    let keypair = Keypair::generate(&mut OsRng);
    let signature = keypair.sign(&file);

    // remove suffix
    let name = input_file_path
        .strip_suffix(INPUT_EXTENSION)
        .unwrap()
        .to_owned();

    // encode results
    let signature_hex = hex::encode(signature.to_bytes());
    let public_key_hex = hex::encode(keypair.public.as_bytes());
    let secret_key_hex = hex::encode(keypair.secret.as_bytes());

    // write results
    fs::write(name.clone() + OUTPUT_EXTENSION, signature_hex).unwrap();
    fs::write(name.clone() + PUB_KEY, public_key_hex).unwrap();
    fs::write(name.clone() + SEC_KEY, secret_key_hex).unwrap();
}
