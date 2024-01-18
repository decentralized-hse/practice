use afl::fuzz;
use std::{fs::{self, File, write, remove_file}, io::Read};
use source_file::run;

mod source_file;

fn ends_correctly(l: &[u8]) -> bool {
    return l.iter().skip_while(|&&b| b != 0).all(|&b| b == 0)
}

fn read(path: &'static str) -> Vec<u8> {
    let metadata = fs::metadata(path).unwrap();
    let mut f = File::open(path).unwrap();
    let mut buffer = vec![0; metadata.len() as usize];
    f.read(&mut buffer).unwrap();
    buffer
}

fn is_correct_input(data: &[u8]) -> bool {
    // 'name' (first 32 bytes of student) must be correct UTF-8 bytes
    // 'login' and 'group' must be correct ASCII bytes
    // 'url' must be correct URL
    // 'mark' must not be NaN
    data.len() % 128 == 0 &&
        data.chunks(128).all(|s|
            std::str::from_utf8(&s[..32]).is_ok() && ends_correctly(&s[..32]) &&
            s[32..48].iter().all(|&b| b < 128) && ends_correctly(&s[32..48]) && !s[32..48].contains(&0x20) &&
            s[48..56].iter().all(|&b| b < 128) && ends_correctly(&s[48..56]) && !s[48..56].contains(&0x20) &&
            s[56..64].iter().all(|&b| b == 0 || b == 1) &&
            std::str::from_utf8(&s[64..64 + 59]).is_ok() && ends_correctly(&s[64..64 + 59]) &&
            !f32::is_nan(f32::from_le_bytes(s[124..128].try_into().unwrap())))
}

fn main() {
    fuzz!(|data: &[u8]| {
        if is_correct_input(data) {
            write("tmp.bin", data.to_vec()).unwrap();
            let source = read("tmp.bin");

            run("tmp.bin".to_string());
            run("tmp.xml".to_string());

            let result = read("tmp.bin");
            assert_eq!(&source, &result);

            remove_file("tmp.bin").unwrap();
            remove_file("tmp.xml").unwrap();
        }
    });
}
