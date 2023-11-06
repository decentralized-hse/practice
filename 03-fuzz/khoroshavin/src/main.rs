use afl::fuzz;
use std::{fs::{self, File, write, remove_file}, io::Read};
use source_file::run;

mod source_file;

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
    // 'url' must be correct URL (= correct UTF-8?) 
    data.len() % 128 == 0 &&
        data.chunks(128).all(|s| std::str::from_utf8(&s[..32]).is_ok() && s[32..56].iter().all(|b| b < &128) && std::str::from_utf8(&s[64..64 + 59]).is_ok())
}

fn main() {
    fuzz!(|data: &[u8]| {
        if is_correct_input(data) {
            let vec = data.to_vec();
            write("tmp.bin", vec).unwrap();
            let source = read("tmp.bin");
    
            run("tmp.bin".to_string());
            run("tmp.sqlite".to_string());
    
            let result = read("tmp.bin");
            assert_eq!(&source, &result);
    
            remove_file("tmp.bin").unwrap();
            remove_file("tmp.sqlite").unwrap();
            let _ = remove_file("tmp.sqlite-journal");
        }
    });
}
