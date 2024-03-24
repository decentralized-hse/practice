use std::env;
use std::fs;

const BIN_EXT: &str = ".bin";
const CAPN_EXT: &str = ".capnproto";

use capnproto_smorodinov_alasheev::{bin_to_capnproto, capnproto_to_bin};

fn main() {
    let argv: Vec<String> = env::args().collect();
    assert_eq!(argv.len(), 2);

    let file_path = &argv[1];

    let input = fs::read(file_path).unwrap();

    let (output, output_path) = if let Some(file) = file_path.strip_suffix(BIN_EXT) {
        (bin_to_capnproto(&input), file.to_owned() + CAPN_EXT)
    } else if let Some(file) = file_path.strip_suffix(CAPN_EXT) {
        (capnproto_to_bin(&input), file.to_owned() + BIN_EXT)
    } else {
        panic!("unknown file extension");
    };

    fs::write(output_path, &output).unwrap();
}
