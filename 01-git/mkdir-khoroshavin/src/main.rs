use std::{env, path::PathBuf, fs};
use sha256::digest;

fn read_file_into_lines(file_name: &str) -> Vec<String> {
    return fs::read_to_string(&file_name)
        .unwrap_or_else(|_| panic!("File with name '{}' must exist", &file_name))
        .lines()
        .map(String::from)
        .collect();
}

struct MkdirArgs {
    root_hash: String,
    dir_path: PathBuf
}

fn get_args() -> MkdirArgs {
    let args: Vec<String> = env::args().collect();

    if args.len() != 3 {
        panic!("Usage: {} <new_dir_path> <root_hash>", args[0]);
    }

    return MkdirArgs {
        root_hash: (&args[2]).to_string(), 
        dir_path: PathBuf::from((&args[1]).trim_start_matches(&['/', '\\']))
    };
}

fn add_new_dir(root_hash: String, dir_path: PathBuf) -> String {
    return update_tree(Some(&root_hash), dir_path.iter());
}

fn get_next_hash<'a>(lines: &'a Vec<String>, next_dir: &str) -> Option<&'a str> {
    let get_hash_from_line = |x: &'a String| -> Option<&str> { 
        (&&x).split("/\t").nth(1)
    };

    return lines
        .iter()
        .find(|x: &&String| (&&x).starts_with(next_dir))
        .and_then(get_hash_from_line);
}

fn create_file_and_get_name(lines: &Vec<String>) -> String {
    let single_line_contents = lines.join("\n");
    let bytes: &[u8] = single_line_contents.as_bytes();
    let hash: String = digest(bytes);
    fs::write(&hash, bytes)
        .expect("Cannot write file, OS error");
    return hash;
}

fn with_new_hash<'a>(lines: &'a mut Vec<String>, dir_name: &str, new_hash: String) -> &'a mut Vec<String> {
    let new_line = format!("{}/\t{}", dir_name, new_hash);
    let line_number: Option<usize> = lines
        .iter()
        .position(|x| x.starts_with(dir_name));

    match line_number {
        None => lines.push(new_line),
        Some(line) => lines[line] = new_line
    };

    return lines;
}

fn update_tree(current_hash: Option<&str>, mut dir_sequence: std::path::Iter<'_>) -> String {
    let next_dir: Option<&str> = dir_sequence.next()
        .and_then(|x: &std::ffi::OsStr| x.to_str());

    if next_dir == None {
        if current_hash.is_some() {
            panic!("This folder exists already")
        } else {
            return create_file_and_get_name(&vec![]);
        }
    }
    
    let mut lines: Vec<String> = current_hash
        .map(|n: &str| read_file_into_lines(n))
        .unwrap_or_default();
    let dir_name: &str = next_dir.unwrap();
    let next_hash: Option<&str> = get_next_hash(&lines, dir_name);
    let new_hash: String = update_tree(next_hash, dir_sequence);
    return create_file_and_get_name(with_new_hash(&mut lines, &dir_name, new_hash));
}

fn main() {
    let MkdirArgs {root_hash, dir_path} = get_args();
    println!("{}", add_new_dir(root_hash, dir_path));
}