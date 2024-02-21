use std::{path::{PathBuf, Iter}, fs};

use clap::Parser;
use sha256::digest;

#[derive(Parser)]
struct Cli {
    path: PathBuf,
    hash: String,
}

fn iterate_and_remove(mut path_iter: Iter, cur_hash: String) -> Option<String> {
    let next_path_iter = path_iter.next();
    if next_path_iter == None {
        return None;
    }

    let next_dir = next_path_iter.map(|x: &std::ffi::OsStr| x.to_str()).unwrap().unwrap();

    let lines: Vec<String> = fs::read_to_string(cur_hash.clone()).unwrap().lines().map(String::from).collect();

    let next_hash = lines.iter().find(|x: &&String| (&&x).starts_with(next_dir)).and_then(|x: &String| (&&x).split("\t").nth(1)).map(|x: &str| x.to_string());

    let new_hash = iterate_and_remove(path_iter, next_hash.unwrap());

    let mut result = Vec::new();
    for line in &lines {
        if line.starts_with(next_dir) && new_hash != None {
            result.push(format!("{}/\t{}", next_dir, new_hash.clone().unwrap()))
        } else if !line.starts_with(next_dir) {
            result.push((&line).to_string())
        }
    }
    result.push("".to_string());

    let binding = result.join("\n");
    let bytes = binding.as_bytes();
    let hash = digest(bytes);

    fs::write(&hash, bytes);

    return Some(hash);
}

fn main() {
    let args = Cli::parse();

    println!("{}", iterate_and_remove(args.path.iter(), args.hash).unwrap());
}
