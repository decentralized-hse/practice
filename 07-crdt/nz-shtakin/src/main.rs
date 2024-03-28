use std::io::{self, Error, Read};
use std::process;

mod zipint;
mod nz;

fn test_stream() -> Result<(), Error>  {
    let mut tlv_stream = Vec::<u8>::new();
    for i in io::stdin().bytes() {
        tlv_stream.push(i?);
    }
    let mut result = nz::N::new();
    let mut i = 0;
    loop {
        let (n, end) = nz::Nparse_impl(&tlv_stream[i..].to_vec())?;
        result.merge(&n);
        i += end;
        if i == tlv_stream.len() {
            break;
        }
    }
    let tlv = result.tlv();
    let s = nz::Nstring(&tlv)?;
    println!("{}", s);
    return Ok(());
}

fn main() {
    match test_stream() {
        Ok(()) => {},
        Err(e) => {
            eprintln!("{}", e);
            process::exit(-1);
        }
    };
}
