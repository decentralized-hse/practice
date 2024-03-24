#![no_main]

use libfuzzer_sys::fuzz_target;

use std::fs;
use std::fs::File;
use std::io::{Read, Error};

use json_boloninid_shubnikov::solve::bin_to_json;
use json_boloninid_shubnikov::solve::json_to_bin;

fn compare(a_path: &str, b_path: &str) -> Result<bool, Error> {
    let mut file1 = File::open(a_path)?;
    let mut file2 = File::open(b_path)?;

    let mut buffer1 = Vec::new();
    let mut buffer2 = Vec::new();

    file1.read_to_end(&mut buffer1)?;
    file2.read_to_end(&mut buffer2)?;

    return Ok(buffer1 == buffer2);
}

fuzz_target!(|data: &[u8]| {
    if data.is_empty() {
        return;
    }

    let mut path : String = "test.bin".to_string();

    let _ = fs::write("test.bin", data);
    match bin_to_json(&mut path) {
        Ok(_) => {}
        Err(_err) => {
           return;
        }
    }

    let _ = fs::rename("test.json", "out.json");
    let mut json_path : String = "out.json".to_string();
    match json_to_bin(&mut json_path) {
        Ok(_) => {}
        Err(_err) => {
            return
        }
    }
    assert!(compare("test.bin", "out.bin").unwrap());
});
