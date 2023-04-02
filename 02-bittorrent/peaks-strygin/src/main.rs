use std::{
    fs::File,
    io::{self, BufRead, BufReader, Write},
    path::Path,
};

fn hash_tree_from_file(filename: impl AsRef<Path>) -> io::Result<Vec<String>> {
    BufReader::new(File::open(filename)?).lines().collect()
}

fn get_index(i: usize, offset: usize) -> usize {
    if i == 0 {
        return offset << 1;
    }
    (1 << (i + 1)) * offset + (1 << i) - 1
}

fn peaks_from_hash_tree(hash_tree: Vec<String>) -> Vec<String> {
    let max_bits = 32;
    let hash_size = 64;
    let zeros = String::from("0".repeat(hash_size));

    let mut peaks: Vec<String> = Vec::new();
    for _ in 0..max_bits {
        peaks.push(zeros.clone())
    }

    let mut offset = 0;
    for i in (0..max_bits).rev() {
        let index = get_index(i, offset);
        if index < hash_tree.len() && !hash_tree[index].eq(&zeros) {
            peaks[i] = hash_tree[index].clone();
            offset += 1;
        }
        offset <<= 1;
    }

    peaks
}

fn peaks_to_file(peaks: Vec<String>, filename: impl AsRef<Path>) -> io::Result<()> {
    let mut f_new = File::create(filename)?;
    for line in peaks {
        writeln!(&mut f_new, "{}", line)?;
    }
    Ok(())
}

fn main() -> io::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    assert!(args.len() == 2);

    let hash_tree = hash_tree_from_file(&args[1])?;
    let peaks = peaks_from_hash_tree(hash_tree);
    peaks_to_file(peaks, format!("{}.peaks", &args[1]))?;

    Ok(())
}
