#![no_main]
extern crate protobuf_zhukov_afanasyeva;

use protobuf_zhukov_afanasyeva::{
    read_students_from_bin_file,
    write_students_to_bin_file
};

use libfuzzer_sys::fuzz_target;
use std::io::Cursor;

fuzz_target!(|data: &[u8]| {
    let mut input = Cursor::new(data.to_vec());
    let mut output = Cursor::new(Vec::new());

    let students = match read_students_from_bin_file(&mut input) {
        Ok(students) => students,
        _ => {
            // Invalid input
            return;
        }
    };

    write_students_to_bin_file(students, &mut output);

    output.set_position(0);
    let output = output.into_inner();

    // check equality
    assert_eq!(data, &output[..], "Original data does not match result");
});
