use std::env;
use std::io::{BufWriter, Write, BufReader, BufRead, Read};
use std::fs::File;
use std::collections::LinkedList;
use data_encoding::HEXLOWER;
use ring::digest::{Context, SHA256};

use regex::Regex;

struct Opts {
    parent_hash: String,
    file_path_string: String,
}

struct DirEntry {
    name: String,
    hash: String,
    is_dir: bool,
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let opts = process_cli_args(args);
    put(opts);
}

fn validate_path(file_path_str: &str) -> Result<LinkedList<String>, String> {
    let path_vec: LinkedList<String> = file_path_str.split(std::path::MAIN_SEPARATOR).map(|s| String::from(s)).collect();
    for dir in &path_vec {
        if dir == ".parent" { // is it weak?
            return Err(String::from("I caught you. The incident will be submitted to your parents.")); // won't be printed ((
        }
    }
    return Ok(path_vec);
}

fn verify_path_in_fs_snapshot(path: &str, fs_hash: &str) -> (LinkedList<String>, String) {
    let Ok(mut dirs_to_update) = validate_path(path) else {
        panic!("Invalid file path passed");
    };
    
    let Some(filename) = dirs_to_update.pop_back() else {
        panic!("Empty file path passed");
    };

    let mut cur_hash_str = String::from(fs_hash);
    for dir in &dirs_to_update {
        let Ok(entries) = read_dir_by_hash(cur_hash_str.as_str()) else {
            panic!("Corrupted directory {}", cur_hash_str.as_str());
        };
        let mut dir_entry = None;
        for entry in entries {
            if entry.name == dir.as_str() {
                if !entry.is_dir {
                    panic!("Expected '{}' to be dir", entry.name);
                }
                dir_entry = Some(entry);
                break;
            }
        }
        if dir_entry.is_none() {
            panic!("Could not found entry '{}'", dir);
        }
        
        cur_hash_str = dir_entry.unwrap().hash;
    }

    let Ok(entries) = read_dir_by_hash(cur_hash_str.as_str()) else {
        panic!("Corrupted directory {}", cur_hash_str.as_str());
    };
    for entry in entries {
        if entry.name == filename {
            if entry.is_dir {
                panic!("Expected '{}' to be file", entry.name);
            }
        }
    }

    (dirs_to_update, filename) 
}

fn put(opts: Opts) {
    let (dirs_to_update, filename) = verify_path_in_fs_snapshot(opts.file_path_string.as_str(), opts.parent_hash.as_str());
    let Ok(hash) = write_file_to_fs_from_stdin(dirs_to_update, filename.as_str(), opts.parent_hash.as_str()) else {
        panic!("Unexpected error");
    };
    println!("{}", hash);
}

fn write_file_to_fs_from_stdin(dirs_to_update: LinkedList<String>, filename: &str, fs_hash: &str) -> std::io::Result<String> {
    write_file_to_dir(dirs_to_update, filename, fs_hash, true)
}

fn write_file_to_dir(mut dirs_to_update: LinkedList<String>, filename: &str, dir_old_hash: &str, add_parent: bool) -> std::io::Result<String> { 
    let mut dir_entries = read_dir_by_hash(dir_old_hash)?;
    if dirs_to_update.is_empty() {
        let file_hash = write_blob_from_stdin()?;
        let mut found_entry = false;
        for entry in &mut dir_entries {
            if entry.name.as_str() == filename {
                found_entry = true; 
                entry.hash = file_hash.clone();
            }
        }
        if !found_entry {
            dir_entries.push(DirEntry{name: filename.to_string(), is_dir: false, hash: file_hash});
        }
    } else {
        for entry in &mut dir_entries {
            if entry.name.as_str() == dirs_to_update.front().unwrap() {
                dirs_to_update.pop_front();
                let new_hash = write_file_to_dir(dirs_to_update, filename, entry.hash.as_str(), false)?;
                entry.hash = new_hash;
                break;
            }
        }
    }

    if add_parent {
        let mut parent_found = false;
        for entry in &mut dir_entries {
            if entry.name == ".parent" {
                entry.hash = String::from(dir_old_hash);
                parent_found = true;
            }
        }
        if !parent_found {
            dir_entries.push(DirEntry{name: String::from(".parent"), is_dir: true, hash: String::from(dir_old_hash)});
        }
    }
    write_new_dir(dir_entries)
}

fn write_new_dir(mut entries: Vec<DirEntry>) -> std::io::Result<String> {
    let mut content = String::new();
    entries.sort_by_key(|entry| entry.name.clone());
    for entry in entries {
        content.push_str(entry.name.as_str());
        if entry.is_dir {
            content.push_str("/");
        } else {
            content.push_str(":");
        }
        content.push_str("\t");
        content.push_str(entry.hash.as_str());
        content.push_str("\n");
    }
    let mut context = Context::new(&SHA256);
    context.update(content.as_bytes());
    let hash_string = HEXLOWER.encode(context.finish().as_ref());
    
    let mut f = File::create(hash_string.as_str())?;
    f.write_all(content.as_bytes())?;
    Ok(hash_string)
}

fn read_dir_by_hash(hash: &str) -> std::io::Result<Vec<DirEntry>> {
    let f = File::open(hash)?;
    let mut reader = BufReader::new(f);

    let mut line = String::new();
    
    let mut len = reader.read_line(&mut line)?;
    let mut entries = Vec::new();
    while len != 0 {
        let trimmed = line.trim();
        if trimmed.is_empty() {
            continue;
        }

        // else match the pattern
        let Ok(re) = Regex::new(r"^([0-9a-zA-Z._]+)([:/])[ \t]+([0-9a-f]+)$") else {
            eprintln!("Line: '{}' in '{}' does not match the pattern. Skipping", 
                        trimmed, hash);
            continue;
        };

        // TODO: actually, only 1 match is possible
        for (_, [f1, f2, f3]) in re.captures_iter(trimmed).map(|caps| caps.extract()) {
            entries.push(DirEntry {
                name: f1.to_string(), 
                is_dir: f2.chars().nth(0) == Some('/'), 
                hash: f3.to_string()
            });
        }

        line.clear();
        len = reader.read_line(&mut line)?; 
    }

    return Ok(entries);
}

fn write_blob_from_stdin() -> std::io::Result<String> {
    let mut reader = std::io::stdin();
    let mut context = Context::new(&SHA256);
    let mut buf = [0 as u8; 0x1000];
    
    let f = File::create("temporary")?;
    let mut writer = BufWriter::new(f);
    let mut len = usize::MAX;
    while len != 0 {
        len = reader.read(&mut buf)?;
        context.update(&buf[..len]);
        writer.write(&buf[..len])?;
    }
    writer.flush()?;
    drop(writer);
    
    let hash_string = HEXLOWER.encode(context.finish().as_ref());
    std::fs::rename("temporary", hash_string.as_str())?;

    Ok(hash_string)
}

fn process_cli_args(args: Vec<String>) -> Opts {
    Opts {
        parent_hash: args[2].clone(),
        file_path_string: args[1].clone(),
    }
}
