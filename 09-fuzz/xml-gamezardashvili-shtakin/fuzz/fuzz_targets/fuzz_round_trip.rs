#![no_main]

use libfuzzer_sys::fuzz_target;

use std::fs;
use std::fs::File;
use std::io::{self, Read, Error};

use xml_gamezardashvili::format::formats_impl;

fn compare(a_path: &str, b_path: &str) -> Result<bool, Error> {
    let mut a = File::open(a_path)?;
    let mut b = File::open(b_path)?;
    let mut buf1 = [0; 4096];
    let mut buf2 = [0; 4096];
    loop {
        let l1 = a.read(&mut buf1)?;
        let l2 = b.read(&mut buf2)?;
        if l1 != l2 {
            return Ok(false);
        } else if l1 == 0 {
            return Ok(true);
        } else if buf1[..l1] != buf2[..l2] {
            return Ok(false);
        }
    }
}

fuzz_target!(|data: &[u8]| {
    if data.is_empty() {
        return;
    }

    let mut path : String = "test.bin".to_string();
    let filename = & mut path;
    fs::write("test.bin", data);
    match formats_impl(filename) {
        Ok(_) => {
        }
        Err(e) => {
            // println!("Error: {}", e);
            return;
        }
    }
    fs::rename("test.xml", "out.xml");
    let mut xml_path : String = "out.xml".to_string();
    let xml_filename = & mut xml_path;
    match formats_impl(xml_filename) {
        Ok(_) => {
        }
        Err(e) => {
            panic!("Error: {}", e);
        }
    }

    assert!(compare("test.bin", "out.bin").unwrap());
});
