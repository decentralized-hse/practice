use std::env;
use std::collections::HashMap;
use std::fs::File;
use std::io::{self, BufRead};
use std::path::Path;
use std::path::PathBuf;

const FILE_SEPARATOR: &str = ":\t";
const DIR_SEPARATOR:  &str = "/\t";

fn read_at_file(line: &str) -> Option<Vec<&str>> {
    let result: Vec<&str> = line.splitn(2, &FILE_SEPARATOR).collect();
    if result.len() == 2 {
        return Some(result);
    }
    None
}

fn read_at_dir(line: &str) -> Option<Vec<&str>> {
    let result: Vec<&str> = line.splitn(2, &DIR_SEPARATOR).collect();
    if result.len() == 2 {
        return Some(result);
    }
    None
}

fn read_file_hash(line: &str) -> Result<(&str, &str, bool), &'static str> {
    if let Some(splitted_line) = read_at_file(line) {
        return Ok((splitted_line[0], splitted_line[1], false));
    }

    if let Some(splitted_line) = read_at_dir(line) {
        return Ok((splitted_line[0], splitted_line[1], true));
    }

    Err("Bad input")
}

fn read_file_full(hash_file_path: &str) -> io::Result<(HashMap<String, String>, HashMap<String, bool>)> {
    let mut name_to_hash = HashMap::new();
    let mut name_to_type = HashMap::new();

    if hash_file_path.is_empty() {
        return Ok((name_to_hash, name_to_type))
    }
    let path = Path::new(hash_file_path);
    println!("Reading hash file: {:?}", path);
    let file = File::open(path)?;
    let reader = io::BufReader::new(file);

    for line_result in reader.lines() {
        let line = line_result?;
        if line.is_empty() {
            continue;
        }

        match read_file_hash(&line) {
            Ok((obj_name, obj_hash, is_dir)) => {
                if check_system_file(obj_name) {
                    continue;
                }
                name_to_hash.insert(obj_name.to_string(), obj_hash.to_string());
                name_to_type.insert(obj_name.to_string(), is_dir);
            },
            Err(e) => {
                println!("Bad [file|dir]name '{}': {}", line, e);
            }
        }
    }

    Ok((name_to_hash, name_to_type))
}

fn check_system_file(file: &str) -> bool {
    file.starts_with(".") && file.len() > 2
}

fn diff(path: &PathBuf, old_hash: &str, new_hash: &str, inside: bool) -> io::Result<String> {
    let mut diff_str = String::new();
    let (name_to_hash_old, directory_marker_old) = read_file_full(old_hash)?;
    let (mut name_to_hash_new, directory_marker) = read_file_full(new_hash)?;
    for (object_old, hash_old) in name_to_hash_old.iter() {
        if *directory_marker_old.get(object_old).unwrap_or(&false) {
            continue;
        }
        if let Some(directory_marker) = directory_marker.get(object_old) {
            if *directory_marker {
                continue;
            }
        }
        if !name_to_hash_new.contains_key(object_old) || name_to_hash_new.get(object_old) != Some(hash_old) {
            diff_str += &format!("-\t{}\n", path.join(object_old).display());
        }
        name_to_hash_new.remove(object_old);
    }

    for (object_old, hash_old) in name_to_hash_old.iter() {
        if !*directory_marker_old.get(object_old).unwrap_or(&true) {
            continue;
        }
        match directory_marker.get(object_old) {
            Some(dir_new) if *dir_new => {
                let diff_internal = diff(&path.join(object_old), hash_old, name_to_hash_new.get(object_old).unwrap_or(&String::new()), inside)?;
                if diff_internal.is_empty() {
                    continue;
                }
                diff_str += &format!("d\t{}\n", path.join(object_old).display());
                diff_str += &diff_internal;
            },
            _ => continue,
        }
    }

    for (object_new, _hash_new) in name_to_hash_new.iter() {
        if !directory_marker.get(object_new).unwrap_or(&false) {
            diff_str += &format!("+\t{}\n", path.join(object_new).display());
        }
    }

    for (object_new, hash_new) in name_to_hash_new.iter() {
        if !directory_marker.get(object_new).unwrap_or(&false) {
            continue;
        }
        if !*directory_marker_old.get(object_new).unwrap_or(&true) {
            continue;
        }
        let nested_diff = diff(&path.join(object_new), &String::new(),hash_new, inside)?;
        diff_str.push_str(&format!("d\t{}\n", path.join(object_new).display()));
        diff_str.push_str(&nested_diff);
    }

    for (object_old, hash_old) in name_to_hash_old.iter() {
        if !directory_marker_old.get(object_old).unwrap_or(&false) {
            continue;
        }
        if !directory_marker.get(object_old).unwrap_or(&false) {
            continue;
        }
        let nested_diff = diff(&path.join(object_old), hash_old, &name_to_hash_new[object_old], inside)?;
        if nested_diff.is_empty() {
            continue;
        }
        diff_str.push_str(&format!("d\t{}\n", path.join(object_old).display()));
        diff_str.push_str(&nested_diff);
    }


    Ok(diff_str)
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() != 4 {
        return Err(std::io::Error::new(std::io::ErrorKind::InvalidInput, "Usage: diff <path> <old_hash> <new_hash>"));
    }

    let path = PathBuf::from(args[1].to_owned());
    let old_hash = args[2].to_owned();
    let new_hash = args[3].to_owned();

    match diff(&path, &old_hash, &new_hash, true) {
        Ok(diff) => println!("{}", diff),
        Err(e) => println!("Ошибка: {}", e),
    }

    Ok(())
}
