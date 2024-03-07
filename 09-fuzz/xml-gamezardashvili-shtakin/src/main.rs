use std::io::Error;
use std::process;

mod format;

fn main() -> Result<(), Error> {
    let mut argv: Vec<String> = std::env::args().collect();
    if argv.len() < 2 {
        eprintln!("Malformed input");
        process::exit(-1);
    }

    let filename = & mut argv[1];

    match format::formats_impl(filename) {
        Ok(_) => Ok(()),
        Err(_) => {
            // println!("Error: {}", e);
            eprintln!("Malformed input");
            process::exit(-1);
        }
    }
}
