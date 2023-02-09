use sodiumoxide;
use std::fs::File;
use std::io::{prelude::*, BufReader, Read};
use std::io::{Seek, SeekFrom};
use std::process::exit;
use std::{env, vec};

struct Hash {
    pub hash: Vec<u8>,
}

impl Hash {
    pub fn new() -> Self {
        Self {
            hash: vec![0u8, 32],
        }
    }
}

struct Verifier {
    file_root: File,
    file_chunk: File,
    file_peaks: File,
    file_proof: File,
    root: Hash,
    chunk: Hash,
    vec_peaks: Vec<Hash>,
    uncle_hashes: Vec<Hash>,
    block_idx: i64,
    local_block_idx: i64,
    peak_lvl: i64,
}

fn process_branch(branch: &Hash) -> String {
    let hexed = sodiumoxide::hex::encode(&branch.hash);
    let chars: Vec<char> = hexed.chars().collect();
    let hex_str: String = chars.into_iter().collect();
    return hex_str;
}

fn check_branches_chunk_l(left: &Hash, right: &Hash) -> Vec<u8> {
    let st_left = process_branch(left);
    let mut st_right = String::new();

    for i in &right.hash {
        st_right.push(*i as char);
    }

    let combined = format!("{}\n{}\n", st_left, st_right);

    let hashed = sodiumoxide::crypto::hash::sha256::hash(combined.as_bytes());

    return hashed.0.to_vec();
}

fn check_branches_chunk_r(left: &Hash, right: &Hash) -> Vec<u8> {
    let mut st_left = String::new();
    let st_right = process_branch(right);

    for i in &left.hash {
        st_left.push(*i as char);
    }

    let combined = format!("{}\n{}\n", st_left, st_right);

    let hashed = sodiumoxide::crypto::hash::sha256::hash(combined.as_bytes());

    return hashed.0.to_vec();
}

fn read_lines(file: &File) -> Vec<Hash> {
    let mut res = Vec::<Hash>::new();
    let reader = BufReader::new(file);
    for line in reader.lines() {
        let mut hash = Hash::new();
        hash.hash = line.unwrap().as_bytes().to_vec();
        res.push(hash);
    }
    return res;
}

impl Verifier {
    pub fn new(filename: &String, block: &String) -> Self {
        Verifier {
            file_root: File::options()
                .read(true)
                .open(format!("{}.root", filename))
                .unwrap(),
            file_chunk: File::options()
                .read(true)
                .open(format!("{}.{}.chunk", filename, block))
                .unwrap(),
            file_peaks: File::options()
                .read(true)
                .open(format!("{}.peaks", filename))
                .unwrap(),
            file_proof: File::options()
                .read(true)
                .open(format!("{}.{}.proof", filename, block))
                .unwrap(),
            root: Hash::new(),
            chunk: Hash::new(),
            vec_peaks: Vec::new(),
            uncle_hashes: Vec::new(),
            block_idx: block.parse::<i64>().unwrap(),
            local_block_idx: 0,
            peak_lvl: 0,
        }
    }

    fn get_root(&mut self) {
        let mut hex = String::new();
        let mut reader = BufReader::new(&self.file_root);
        reader.read_line(&mut hex).unwrap();
        self.root.hash = sodiumoxide::hex::decode(&hex[0..64]).unwrap();
    }

    fn get_peaks(&mut self) {
        self.vec_peaks = read_lines(&self.file_peaks);
    }

    fn get_hashed_peaks(&self) -> Vec<u8> {
        let mut peaks = Vec::<u8>::new();
        for i in &self.vec_peaks {
            peaks.extend(&i.hash);
            peaks.push('\n' as u8);
        }
        let hashed = sodiumoxide::crypto::hash::sha256::hash(&peaks);
        return hashed.0.to_vec();
    }

    fn get_uncle_hashes(&mut self) {
        self.uncle_hashes = read_lines(&self.file_proof);
    }

    fn get_chunk(&mut self) {
        let mut st = String::new();
        let mut reader = BufReader::new(&self.file_chunk);
        reader.read_to_string(&mut st).unwrap();
        let hashed = sodiumoxide::crypto::hash::sha256::hash(st.as_bytes());
        self.chunk.hash = hashed.0.to_vec();
    }

    pub fn check_peaks(&mut self) -> bool {
        self.get_root();
        self.get_peaks();

        let peak_hash = self.get_hashed_peaks();

        return self.root.hash == peak_hash;
    }

    fn get_block_idx(&mut self) -> Result<i64, i64> {
        let mut j: i64 = 0;
        for i in (0..self.vec_peaks.len()).rev() {
            if self.vec_peaks[i].hash != vec![0u8, 32] {
                j += 1 << i;
                if j > self.block_idx {
                    return Ok(self.block_idx + ((1 as i64) << i) - j);
                }
            }
        }
        return Err(0);
    }

    pub fn check_block_idx(&mut self) -> bool {
        match self.get_block_idx() {
            Ok(res) => {
                self.local_block_idx = res;
                return true;
            }
            Err(_) => return false,
        }
    }

    fn calculate_peak_lvl(&mut self) -> Result<i64, i64> {
        let mut size: i64 = 0;
        for lvl in (0..self.vec_peaks.len()).rev() {
            if self.vec_peaks[lvl].hash != vec!['0' as u8; 64] {
                size += 1 << lvl;
                if size > self.block_idx {
                    return Ok(lvl as i64);
                }
            }
        }
        return Err(0);
    }

    pub fn compare_peak_lvl_uncles(&mut self) -> bool {
        match self.calculate_peak_lvl() {
            Ok(res) => self.peak_lvl = res,
            Err(_) => return false,
        }

        self.get_uncle_hashes();

        return self.uncle_hashes.len() == self.peak_lvl as usize;
    }

    pub fn hashes_verify(&mut self) -> bool {
        self.get_chunk();

        for i in 0..self.peak_lvl {
            if self.local_block_idx % 2 == 0 {
                self.chunk.hash =
                    check_branches_chunk_l(&self.chunk, &self.uncle_hashes[i as usize]);
            } else {
                self.chunk.hash =
                    check_branches_chunk_r(&self.uncle_hashes[i as usize], &self.chunk);
            }
            self.local_block_idx /= 2;

            let hexed = sodiumoxide::hex::encode(&self.chunk.hash);
            let chars: Vec<char> = hexed.chars().collect();
            let chunk_str: String = chars.into_iter().collect();

            let mut peak_str = String::new();
            for i in &self.vec_peaks[(i + 1) as usize].hash {
                peak_str.push(*i as char);
            }

            if peak_str != chunk_str {
                return false;
            }
        }
        return true;
    }
}

fn main() {
    let argv: Vec<String> = env::args().collect();

    if argv.len() != 3 {
        panic!("Incorrect number of arguments");
    }

    match sodiumoxide::init() {
        Ok(_) => {}
        Err(_) => {
            panic!("Unable to init libsodium");
        }
    }

    let mut ver = Verifier::new(&argv[1], &argv[2]);

    if !ver.check_peaks() {
        println!("Wrong peaks");
        exit(1);
    }
    if !ver.check_block_idx() {
        println!("Wrong block index");
        exit(1);
    }
    if !ver.compare_peak_lvl_uncles() {
        println!("Wrong uncles");
        exit(1);
    }
    if !ver.hashes_verify() {
        println!("Wrong proof");
        exit(1);
    }
    println!("OK");
    exit(0);
}
