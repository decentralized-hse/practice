use std::fs;
use std::io::Read;
use std::path::{Path, PathBuf};
use crate::{Args, ArgsError, IntegrityError};
use sha2::Sha256;
use sha2::Digest;

pub fn validate_args(row_args: Vec<String>) -> Result<Args, ArgsError> {
    if !matches!(row_args.len(), 3) {
        return Err(ArgsError {
            info: "There should be 3 arguments".to_string(),
        });
    }

    if !matches!(row_args[2].len(), 64) {
        return Err(ArgsError {
            info: "The third argument should be a Sha256 hash".to_string(),
        });
    }
    let args = Args {
        hash: row_args[2].clone(),
        path: row_args[1].split('/').map(PathBuf::from).collect(),
    };
    Ok(args)
}

fn read_file_content(hash: &str) -> Result<String, Box<dyn std::error::Error>> {
    let mut file_content = String::new();
    fs::File::open(hash)
        .and_then(|mut file| file.read_to_string(&mut file_content))
        .map_err(|_| Box::new(ArgsError {
            info: format!("File {:?} not found", hash),
        }) as Box<dyn std::error::Error>)
        .map(|_| file_content)
}

pub fn validate_file_or_dir(file_or_dir: &Path, is_dir: bool) -> Result<bool, ArgsError> {
    let file_content = read_file_content(file_or_dir.to_str().ok_or_else(|| ArgsError {
        info: format!("Invalid path: {:?}", file_or_dir),
    })?)?;

    let file_hash = Sha256::digest(file_content.as_bytes());
    let expected_hash = hex::encode(file_hash);

    if file_or_dir.to_string_lossy() != expected_hash {
        eprintln!("Incorrect tree, file hash {:?} is incorrect", file_or_dir);
        return Ok(false);
    }

    if is_dir {
        let lines: Vec<&str> = file_content.lines().collect();

        for line in lines {
            if line.is_empty() {
                break;
            }

            let (separator, is_dir) = match (line.contains("/\t"), line.contains(":\t")) {
                (true, false) => ("/\t", true),
                (false, true) => (":\t", false),
                _ => {
                    eprintln!("Incorrect tree, directory with hash {:?} has incorrect format", file_or_dir);
                    return Ok(false);
                }
            };

            let name_and_hash: Vec<&str> = line.split(separator).collect();

            let fl = validate_file_or_dir(Path::new(name_and_hash[1].trim()), is_dir)?;

            if !fl {
                return Ok(false);
            }
        }
    }

    Ok(true)
}

fn find_dir_hash(file_content: &str, dir_name: &str) -> Result<String, ArgsError> {
    for line in file_content.lines() {
        if let [name, hash] = line.split("/\t").collect::<Vec<&str>>().as_slice() {
            if name.trim() == dir_name {
                return Ok(hash.trim().to_string());
            }
        }
    }
    Err(ArgsError {
        info: format!("Directory {} not found", dir_name),
    })
}

pub fn validate_tree(args: &Args) -> Result<(), Box<dyn std::error::Error>> {
    let mut hash = args.hash.clone();

    for (i, path_element) in args.path.iter().enumerate() {
        let file_content = read_file_content(&hash)?;

        let file_hash = Sha256::digest(file_content.as_bytes());
        let expected_hash = hex::encode(file_hash);

        if hash != expected_hash {
            return Err(Box::new(IntegrityError {
                info: format!("Integrity violation in file {:?}", &hash),
            }));
        }

        let dir_hash = find_dir_hash(&file_content, path_element.to_str().ok_or_else(|| ArgsError {
            info: format!("Invalid path: {:?}", path_element),
        })?)?;
        if i == args.path.len() - 1 {
            let is_valid = validate_file_or_dir(Path::new(&dir_hash), true)?;
            if is_valid {
                println!("Tree is valid");
            }
        } else {
            hash = dir_hash;
        }
    }

    Ok(())
}