use sodiumoxide;
use std::fs::File;
use std::io::{prelude::*, BufReader, Read};
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
    let mut chars: Vec<char> = hexed.chars().collect();
    *chars.last_mut().unwrap() = '\n';
    let hex_str: String = chars.into_iter().collect();
    return hex_str;
}

fn check_branches(left: &Hash, right: &Hash) -> Vec<u8> {
    let mut state = sodiumoxide::crypto::hash::State::new();

    state.update(process_branch(left).as_bytes());
    state.update(process_branch(right).as_bytes());
    let hashed = state.finalize();

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
        let reader = BufReader::new(&self.file_peaks);
        for line in reader.lines() {
            let hash = line.unwrap();
            let hash = hash.as_bytes();
            peaks.extend_from_slice(hash);
        }
        let hashed = sodiumoxide::crypto::hash::sha256::hash(&peaks);
        return hashed.0.to_vec();
    }

    fn get_uncle_hashes(&mut self) {
        self.uncle_hashes = read_lines(&self.file_proof);
    }

    fn get_chunk(&mut self) {
        let mut chunk = vec![0u8; 1024];
        let mut reader = BufReader::new(&self.file_chunk);
        reader.read_exact(&mut chunk).unwrap();
        let hashed = sodiumoxide::crypto::hash::sha256::hash(&chunk);
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
        for i in (self.vec_peaks.len() - 1)..=0 {
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
        for lvl in self.vec_peaks.len() - 1..=0 {
            if self.vec_peaks[lvl].hash != vec![0u8, 32] {
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
                self.chunk.hash = check_branches(&self.chunk, &self.uncle_hashes[i as usize]);
            } else {
                self.chunk.hash = check_branches(&self.uncle_hashes[i as usize], &self.chunk);
            }
            self.local_block_idx /= 2;
        }
        return self.chunk.hash == self.vec_peaks[self.peak_lvl as usize].hash;
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
