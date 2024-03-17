use std::env;
use std::fs;
use std::process;
use std::path::Path;
use std::io::{BufRead, BufReader};
use std::io::Read;

const COMMIT_TEMPLATE: &str = "Root:";
const COMMIT_HASH: &str = ".commit:";
const PARENT_HASH: &str = ".parent/";

fn get_line_by_prefix(content: &Vec<String>, prefix: &str) -> String {
    for line in content.iter() {
        if line.starts_with(prefix) {
            return line[prefix.len()..].trim().to_string();
        }
    }
    String::new()
}

fn get_line_from_file(filename: &str, prefix: &str) -> String {
    let file = fs::File::open(&filename).expect("failed to open file");
    let reader = BufReader::new(file);
    let content: Vec<String> = reader.lines().map(|line| line.unwrap()).collect();
    get_line_by_prefix(&content, prefix)
}

fn print_contents(file: &String) {    
    if !fs::metadata(file).is_ok() {
        println!("broken hash {}", file);
        std::process::exit(1);
    }
    let file_fd = fs::File::open(file).expect("failed to open file");
    let mut reader = BufReader::new(file_fd);
    let first_line = reader.by_ref().lines().next().unwrap().unwrap();
    if first_line.starts_with(COMMIT_TEMPLATE) {
        println!("{}", first_line);
        reader.by_ref().lines().next(); // date
        for line in reader.lines() {
            println!("{}", line.unwrap());
        }
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        println!("provide some hash");
        process::exit(1);
    }
    let mut hash = args[1].to_string();
    let mut prev_commit = "".to_string();
    loop {
        let path = Path::new(&hash);
        if !path.exists() {
            println!("path does not exists");
            process::exit(1);
        }

        let parent = get_line_from_file(&hash, PARENT_HASH);
        if parent.len() == 0 {
            break;
        }

        let commit = get_line_from_file(&hash, COMMIT_HASH);
        if commit.len() != 0 && commit != prev_commit {
            print_contents(&commit);
            prev_commit = commit;
        }
        hash = parent.clone();
    }
}
