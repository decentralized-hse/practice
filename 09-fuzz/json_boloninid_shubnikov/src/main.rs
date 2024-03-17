use std::env;
use std::process;

mod solve;

fn main() {
    let mut argv: Vec<String> = env::args().collect();

    if argv.len() < 2 {
        panic!("Cannot find path to file");
    }

    if argv[1].ends_with(".json") {
        match solve::json_to_bin(&mut argv[1]) {
            Ok(_) => {
                println!("Ok!");
            }
            Err(err) => {
                eprintln!("Error {}", err);
                process::exit(-1);
            }
        }
    } else {
        match solve::bin_to_json(&mut argv[1]) {
            Ok(_) => {
                println!("Ok!");
            }
            Err(err) => {
                eprintln!("Error {}", err);
                process::exit(-1);
            }
        }
    }
}
