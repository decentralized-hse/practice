#![no_main]

use libfuzzer_sys::fuzz_target;

use std::fs;

use xml_gamezardashvili::format::formats_impl;

fuzz_target!(|data: &[u8]| {
    let mut path : String = "test.xml".to_string();
    let filename = & mut path;
    fs::write("test.xml", data);
    formats_impl(filename);
});
