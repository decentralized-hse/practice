use std::env;
use std::fs;
use std::path::Path;

fn get(path: &str, root_hash: &str) -> Option<String> {
    let root_path = Path::new(root_hash);

    let target_path = root_path.join(path);

    if !target_path.exists() {
        return None;
    }

    let content = fs::read_to_string(&target_path).ok()?;
    Some(content)
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
            if let Some(content) = get(path, root_hash) {
                println!("{}", content);
            } else {
                eprintln!("Error: Couldn't fetch content for {}", path);
            }
        }
        _ => eprintln!("Unsupported command"),
    }
}
