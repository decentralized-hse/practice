use std::io::Write;
use data_encoding::HEXLOWER;
use ring::digest::{digest, SHA256, Context, SHA256_OUTPUT_LEN};

const CHUNK_SIZE: usize = 1024;
const EMPTY_HASH: [u8; SHA256_OUTPUT_LEN] = [0u8; SHA256_OUTPUT_LEN];

fn range(i: usize) -> std::ops::Range<usize> {
    i * SHA256_OUTPUT_LEN..(i + 1) * SHA256_OUTPUT_LEN
}

fn combine(l: &[u8], r: &[u8]) -> [u8; SHA256_OUTPUT_LEN] {
    if l == EMPTY_HASH || r == EMPTY_HASH {
        return EMPTY_HASH;
    }
    let mut ctx = Context::new(&SHA256);
    ctx.update(HEXLOWER.encode(l).as_ref());
    ctx.update(&['\n' as u8]);
    ctx.update(HEXLOWER.encode(r).as_ref());
    ctx.update(&['\n' as u8]);
    ctx.finish().as_ref().try_into().unwrap()
}

fn seq(l: usize, k: usize) -> usize {
    match l {
        0 => 2 * k,
        _ => (1 << (l + 1)) * k + (1 << l) - 1
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let path = &args.get(1).ok_or("Not enough arguments")?;
    let bytes = std::fs::read(path)?;
    assert_eq!(bytes.len() % CHUNK_SIZE, 0, "File size must be a multiple of {CHUNK_SIZE}");

    let chunks = bytes.chunks_exact(CHUNK_SIZE);

    let mut hashes = vec![0u8; (2 * chunks.len() - 1) * SHA256_OUTPUT_LEN];
    for (i, chunk) in chunks.enumerate() {
        hashes[range(seq(0, i))].copy_from_slice(digest(&SHA256, chunk).as_ref());
    }

    let len = hashes.len() / SHA256_OUTPUT_LEN;
    for l in (1usize..).take_while(|&l| seq(l, 0) < len) {
        for k in (0usize..).take_while(|&k| seq(l, k) < len) {
            let left_seq = seq(l - 1, 2 * k);
            let right_seq = seq(l - 1, 2 * k + 1);

            let &left_child = &hashes.get(range(left_seq)).unwrap_or(&EMPTY_HASH);
            let &right_child = &hashes.get(range(right_seq)).unwrap_or(&EMPTY_HASH);

            let combined_hash = combine(left_child, right_child);

            hashes[range(seq(l, k))].copy_from_slice(combined_hash.as_slice())
        }
    }

    let mut out = std::io::BufWriter::new(
        std::fs::File::create(format!("{path}.hashtree"))?);

    for hash in hashes.chunks_exact(SHA256_OUTPUT_LEN) {
        writeln!(out, "{}", HEXLOWER.encode(hash))?;
    }

    Ok(())
}
