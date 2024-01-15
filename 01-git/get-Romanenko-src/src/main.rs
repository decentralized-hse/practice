use std::env;
use std::fs;
use std::process;

fn is_valid_char(c: char) -> bool {
    !(c.is_whitespace() || c == '\t' || c == ':')
}

fn read_tree(file_hash: &str) -> Result<String, std::io::Error> {
    fs::read_to_string(file_hash)
}

fn get_object_index(lines: &[&str], target_name: &str) -> Option<usize> {
    lines.iter().position(|&line| {
        let name = line.split('\t').next().unwrap();
        let name = &name[..name.len() - 1];
        name == target_name
    })
}

fn read_object_from_tree_by_index(
    path_objects_names: &[String],
    index: usize,
    tree: &[&str],
) -> Result<String, String> {
    let data_split: Vec<&str> = tree[index].split('\t').collect();
    let name = data_split[0];
    let hash = data_split[1];

    match name.chars().last().unwrap() {
        '/' => {
            if !path_objects_names.is_empty() {
                let new_hash = read_object_from_tree(path_objects_names, hash)?;
                Ok(tree[index].replace(hash, &new_hash))
            } else {
                Ok(tree[index].to_string())
            }
        }
        ':' => {
            if path_objects_names.is_empty() {
                fs::read_to_string(hash).map_err(|e| e.to_string())
            } else {
                Err("File not found".to_string())
            }
        }
        _ => Err("Invalid tree structure".to_string()),
    }
}

fn read_object_from_tree(path_objects_names: &[String], parent_hash: &str) -> Result<String, String> {
    let tree = read_tree(parent_hash).map_err(|e| e.to_string())?;
    let tree_lines: Vec<&str> = tree.split('\n').collect();
    let object_tree_index = get_object_index(&tree_lines, &path_objects_names[0])
        .ok_or("File not in tree".to_string())?;

    read_object_from_tree_by_index(
        &path_objects_names[1..],
        object_tree_index,
        &tree_lines,
    )
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() != 3 {
        // Первый аргумент (args[0]) обычно содержит имя запущенной программы.
        eprintln!("Exactly 2 arguments required");
        process::exit(1);
    }

    let path = &args[1];
    let hash = &args[2];

    if hash.len() != 64 {
        eprintln!("Invalid SHA-256 hash");
        process::exit(1);
    }

    if path.chars().any(|c| !is_valid_char(c)) {
        eprintln!("Invalid characters in path");
        process::exit(1);
    }

    let path_objects_names: Vec<String> = path.split('/').map(String::from).collect();

    if path_objects_names.iter().any(|name| name.is_empty()) {
        eprintln!("Some object name in the path is empty");
        process::exit(1);
    }

    match read_object_from_tree(&path_objects_names, hash) {
        Ok(contents) => println!("{}", contents),
        Err(e) => {
            eprintln!("Error: {}", e);
            process::exit(1);
        }
    }
}
