#![no_main]
#[macro_use] extern crate libfuzzer_sys;

use capnproto_smorodinov_alasheev::{bin_to_capnproto, validate_input, capnproto_to_bin};

fuzz_target!(|data: &[u8]| {
    if !validate_input(data) {
        return;
    }
    let old = bin_to_capnproto(data);

    let new = capnproto_to_bin(old.clone().as_slice());

    assert_eq!(data, new);
});
