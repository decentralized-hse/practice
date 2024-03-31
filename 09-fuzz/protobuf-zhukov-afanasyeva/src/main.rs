use prost::{
    bytes::{BufMut, BytesMut},
    Message,
};
use protobuf_zhukov_afanasyeva::binformat::Students;
use protobuf_zhukov_afanasyeva::{
    read_students_from_bin_file,
    write_students_to_bin_file
};
use core::panic;
use std::{env, fs, io};


const EXT_BIN: &str = ".bin";
const EXT_PROTO: &str = ".protobuf";

fn read_students_from_protobuf_file(f: &mut impl io::Read) -> Students {
    println!("[*] Reading protobuf student data...");
    let mut b = BytesMut::new().writer();
    io::copy(f, &mut b).expect("[x] Can't read proto file");
    Students::decode(b.into_inner()).expect("[x] Can't decode proto file")
}

fn write_students_to_protobuf_file(students: Students, f: &mut impl io::Write) {
    let mut buf: Vec<u8> = Vec::with_capacity(300);
    students.encode(&mut buf).unwrap();
    f.write_all(&buf).expect("[x] Can't write to proto file")
}

fn main() { 
    let argv: Vec<String> = env::args().collect();
    assert!(argv.len() == 2, "[x] Incorrect number of arguments");

    let input_filename = argv[1].clone();

    let mut ext_from = EXT_BIN;
    let mut ext_to = EXT_PROTO;
    if input_filename.ends_with(EXT_PROTO) {
        (ext_from, ext_to) = (ext_to, ext_from);
    } else if !input_filename.ends_with(EXT_BIN) {
        panic!("[x] Unexpected file extension");
    }

    let output_filename = input_filename.rsplit_once('.').unwrap().0.to_string() + ext_to;
    
    let mut ifile = fs::File::open(input_filename).expect("[x] Can't open file");
    let students: Students;
    if ext_from == EXT_BIN {
        students = read_students_from_bin_file(&mut ifile).unwrap();
    } else {
        students = read_students_from_protobuf_file(&mut ifile);
    }
    println!("[*] {} student(s) read...", students.student.len());

    println!("[*] Writing to {} ...", &output_filename);
    let mut ofile = fs::OpenOptions::new().create(true).write(true).open(&output_filename).expect("[x] Can't create/open file");
    if ext_to == EXT_PROTO {
        write_students_to_protobuf_file(students, &mut ofile)
    } else {
        write_students_to_bin_file(students, &mut ofile)
    }
    println!("[v] Done!")
}
