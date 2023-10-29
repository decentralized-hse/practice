use std::env;
use std::fs;
use std::io;

fn get(path: &str, root_hash: &str) -> io::Result<String> {
    let parts: Vec<&str> = path.split('/').collect();

    let mut current_hash = root_hash.to_string();

    for part in parts {
        let content = fs::read_to_string(&current_hash)?;

        if let Some(line) = content.lines().find(|line| line.starts_with(part)) {
            let parts: Vec<&str> = line.split('\t').collect();
            if parts.len() < 2 {
                return Err(io::Error::new(io::ErrorKind::InvalidData, "Invalid data format"));
            }
            current_hash = parts[1].to_string();
        } else {
            return Err(io::Error::new(io::ErrorKind::NotFound, format!("{} not found", part)));
        }
    }

    let content = fs::read_to_string(&current_hash)?;
    Ok(content)
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() != 4 {
        eprintln!("Usage: {} <command> <path> <root_hash>", args[0]);
        return;
    }

    let command = &args[1];
    let path = &args[2];
    let root_hash = &args[3];

    match command.as_str() {
        "get" => {
            match get(path, root_hash) {
                Ok(content) => println!("{}", content),
                Err(e) => eprintln!("Error: {}", e),
            }
        }
        _ => eprintln!("Unsupported command"),
    }
}
