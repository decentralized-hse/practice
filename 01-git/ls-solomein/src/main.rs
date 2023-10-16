use std::{env, str};
use std::borrow::Cow;
use std::fmt::format;
use std::fs::File;
use std::io::{self, BufRead, BufReader, Error, Read};
use sha256::{digest, try_digest};


fn find_target_dir_meta_file<'a>(path: &Vec<&str>, root_blob: &'a str) -> Result<String, Error> {
    let mut current_blob = root_blob.to_owned();
    for name in path {
        if format!("{}", name) == "" {
            return Ok(current_blob.to_owned());
        }
        let current_blobb = resolve_directory_blob_by_name(&current_blob, name)
            .expect("Invalid structure");

        current_blob = current_blobb
    }

    return Ok(current_blob.to_owned());
}

fn resolve_directory_blob_by_name(current_blob: &str, name: &str) -> Result<String, io::Error> {
    let file = File::open(current_blob)?;
    let reader = BufReader::new(file);
    let lines: Vec<String> = reader.lines().collect::<Result<_, _>>()?;
    for line in lines {
        let mut line_view: Vec<&str> = line.split("/\t").collect();
        if line_view.len() < 2 {
            continue;
        }
        if line_view[0] == name {
            return Ok(format!("{}", line_view[1]));
        }
    }
    Err(io::Error::new(io::ErrorKind::NotFound, "Line not found in the file"))
}

fn print_directory_tree_rec(start_blob: &str, depth: usize) -> Result<(), Error> {
    let file = File::open(start_blob)?;
    let reader = BufReader::new(file);
    let lines: Vec<String> = reader.lines().collect::<Result<_, _>>()?;
    for line in lines {
        println!("{}", format!("{}{}", "\t".repeat(depth), line));
        if line.contains("/\t") {
            let view: Vec<&str> = line.split("/\t").collect();
            print_directory_tree_rec(view[1], depth + 1).expect("");
        } else {
            let blob_name: Vec<&str> = line.split(":\t").collect();
            let blob = File::open(blob_name[1])?;
            let blob_reader = BufReader::new(blob);
            let l: Vec<String> = blob_reader.lines().collect::<Result<_, _>>()?;
            if digest(l.join("\n")) != blob_name[1] {
                return Err(io::Error::new(io::ErrorKind::NotFound, "Несовпадение хеша, файловая система повреждена"));
            }
        }
    }
    let blob = File::open(start_blob)?;
    let blob_reader = BufReader::new(blob);
    let l: Vec<String> = blob_reader.lines().collect::<Result<_, _>>()?;
    if digest(l.join("\n")) != start_blob {
        return Err(io::Error::new(io::ErrorKind::NotFound, "Несовпадение хеша, файловая система повреждена"));
    }
    return Ok(());
}


fn main() {
    // Получите аргументы командной строки в виде вектора строк.
    let args: Vec<String> = env::args().collect();

    // Первый аргумент (args[0]) обычно содержит имя запущенной программы.
    // Если вас интересуют только дополнительные аргументы, начните с args[1].
    if args.len() < 3 {
        eprintln!("Usage: {} <arg1> <arg2>", args[0]);
        std::process::exit(1);
    }

    // Ваши аргументы находятся в векторе args.
    let target_dir_path = &args[1];
    let root_hash = &args[2];

    if target_dir_path == "." {
        let recursive_result = print_directory_tree_rec(&root_hash, 0)
            .expect("unexpected error");
        return;
    }
    let path_to_dir: Vec<&str> = target_dir_path.split('/').collect();
    let target_dir_hash = find_target_dir_meta_file(&path_to_dir, root_hash);
    let recursive_result = print_directory_tree_rec(&target_dir_hash.expect("unexpected error"), 0)
        .expect("unexpected error");
}
