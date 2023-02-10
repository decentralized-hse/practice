use std::env;
use std::fs::File;
use std::io::{prelude::*, BufReader, BufWriter};
use std::fs::OpenOptions;

fn read_file(path: &str) -> Vec<String> {
    let file_name = format!("{path}.hashtree");
    let file = File::open(file_name.clone());
    if file.is_err() {
        panic!("It is impossible to open file {file_name}, reason: {:?}", file.err());
    }
    let reader = BufReader::new(file.unwrap());
    let mut ret = vec!();
    for line in reader.lines() {
        if line.is_err() {
            continue;
        }
        ret.push(line.unwrap())
    }

    ret
}

fn process(hashes: Vec<String>, index: usize) -> Vec<String> {
    let n = hashes.len();
    let mut ret = vec!();
    let mut i = 1usize;
    let node = index << 1;
    loop {
        let p = 1 << i;
        let par = (node + (node ^ p)) / 2;
        if par + p - 1 >= n {
            break;
        }
        ret.push(hashes[node ^ p].clone());
        i += 1;
    }
    ret
}

fn write_to_file(file_name: &str, data: Vec<String>) {
    let file = OpenOptions::new().create(true).write(true).open(file_name);
    if file.is_err() {
        panic!("It is impossible to open file {file_name}");
    }
    let mut writer = BufWriter::new(file.unwrap());
    for i in data {
        writer.write(i.as_bytes()).expect("");
    }
    writer.flush().expect("Error during flush to file {file_name.to_string()}");
}

pub fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        panic!("Not enough args");
    }
    let t = read_file(&args[1]);
    let data = process(t, args[2].parse::<usize>().unwrap());
    write_to_file(&format!("{}.{}.proof", &args[1], &args[2]), data);
}
