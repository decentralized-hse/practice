use std::fs;
use sha2::Digest;

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() != 4 {
        eprintln!("Поданы неправильные аргументы на вход. Должно быть 3 аргумента");
        std::process::exit(1);
    }

    let path: Vec<&str> = args[2].split('/').collect();
    let hash = &args[3];

    let mut current_hash = hash.clone();
    for (i, segment) in path.iter().enumerate() {
        let file_content = match fs::read(&current_hash) {
            Ok(content) => content,
            Err(e) => {
                eprintln!("Ошибка: {}", e);
                std::process::exit(1);
            }
        };

        let calculated_hash = format!("{:x}", sha2::Sha256::digest(&file_content));
        if calculated_hash != current_hash {
            eprintln!("Целостность была нарушена. Целостность Данного файла была нарушена");
            std::process::exit(1);
        }

        if i == path.len() - 1 {
            match find_file(&file_content, segment) {
                Some(filename) => {
                    if let Err(e) = print_file(&filename) {
                        eprintln!("Ошибка: {}", e);
                        std::process::exit(1);
                    }
                }
                None => {
                    eprintln!("Файл {} не найден", segment);
                    std::process::exit(1);
                }
            }
        } else {
            match find_dir(&file_content, segment) {
                Some(dirhash) => current_hash = dirhash,
                None => {
                    eprintln!("Директория {} не найдена", segment);
                    std::process::exit(1);
                }
            }
        }
    }
}

fn print_file(filename: &str) -> std::io::Result<()> {
    let file_content = fs::read_to_string(filename)?;
    for line in file_content.lines() {
        println!("{}", line);
    }
    Ok(())
}

fn find_file(file_content: &[u8], filename: &str) -> Option<String> {
    let content_str = String::from_utf8_lossy(file_content);
    for line in content_str.lines() {
        let parts: Vec<&str> = line.split(":\t").collect();
        if parts.len() == 2 && parts[0].trim() == filename {
            return Some(parts[1].trim().to_string());
        }
    }
    None
}

fn find_dir(file_content: &[u8], dir_name: &str) -> Option<String> {
    let content_str = String::from_utf8_lossy(file_content);
    for line in content_str.lines() {
        let parts: Vec<&str> = line.split("/\t").collect();
        if parts.len() == 2 && parts[0].trim() == dir_name {
            return Some(parts[1].trim().to_string());
        }
    }
    None
}
