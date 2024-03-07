#![no_main]

use libfuzzer_sys::fuzz_target;

use std::fs;

use xml_gamezardashvili::format::formats_impl;

fuzz_target!(|data: &[u8]| {
    let mut path : String = "test.bin".to_string();
    let filename = & mut path;
    fs::write("test.bin", data);
    formats_impl(filename);
});
