use std::io::Error;

mod format;

fn main() -> Result<(), Error> {
    let mut argv: Vec<String> = std::env::args().collect();
    if argv.len() < 2{
        panic!("NO file provided!");
    }

    let filename = & mut argv[1];

    match format::formats_impl(filename) {
        Ok(_) => Ok(()),
        Err(e) => {
            println!("Error: {}", e);
            return Ok(());
        }
    }
}
