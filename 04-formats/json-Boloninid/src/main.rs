use arrayvec::ArrayVec;
use serde::{Deserialize, Serialize};
use serde_json::Result;
use std::fs::write;
use std::fs::File;
use std::io::{BufReader, Read};
use std::{env, vec};

const SIZE: usize = 128;

struct MarkU8 {
    repo: [u8; 59],
    mark: u8,
}

struct StudentU8 {
    //size = 128, align = 8
    name: [u8; 32],
    login: [u8; 16],
    group: [u8; 8],
    practice: [u8; 8],
    project: MarkU8,
    mark: f32,
}

impl MarkU8 {
    pub fn new() -> Self {
        MarkU8 {
            repo: [0; 59],
            mark: 0u8,
        }
    }
}

impl StudentU8 {
    pub fn new() -> Self {
        StudentU8 {
            name: [0; 32],
            login: [0; 16],
            group: [0; 8],
            practice: [0u8; 8],
            project: MarkU8::new(),
            mark: 0.0,
        }
    }
}

#[derive(Serialize, Deserialize)]
struct Mark {
    repo: String,
    mark: u8,
}

#[derive(Serialize, Deserialize)]
struct Student {
    //size = 128, align = 8
    name: String,
    login: String,
    group: String,
    practice: [u8; 8],
    project: Mark,
    mark: f32,
}

impl Student {
    pub fn from_u8(init: StudentU8) -> Self {
        Student {
            name: String::from_utf8_lossy(&init.name).to_string(),
            login: String::from_utf8_lossy(&init.login).to_string(),
            group: String::from_utf8_lossy(&init.group).to_string(),
            practice: init.practice,
            project: Mark {
                repo: String::from_utf8_lossy(&init.project.repo).to_string(),
                mark: init.project.mark,
            },
            mark: init.mark,
        }
    }
}

fn bin_to_json(filename: &String) {
    let inp = File::options().read(true).open(filename).unwrap();

    let mut reader = BufReader::new(inp);

    let mut bytes = Vec::new();

    match reader.read_to_end(&mut bytes) {
        Ok(_) => {}
        Err(_) => {
            panic!("Failed to read binary file");
        }
    }

    let dst: Vec<&[u8]> = bytes.chunks(SIZE).collect();

    let mut result: Vec<Student> = vec![];

    for i in 0..dst.len() {
        let mut s = StudentU8::new();

        let slice = dst[i];
        let array: ArrayVec<&u8, SIZE> = slice.into_iter().collect();
        let bytes: [&u8; SIZE] = array.into_inner().unwrap();

        let mut bytes_fin = [0u8; SIZE];
        for i in 0..bytes.len() {
            bytes_fin[i] = *bytes[i];
        }

        s = StudentU8 {
            name: bytes_fin[0..32].try_into().unwrap(),
            login: bytes_fin[32..48].try_into().unwrap(),
            group: bytes_fin[48..56].try_into().unwrap(),
            practice: bytes_fin[56..64].try_into().unwrap(),
            project: MarkU8 {
                repo: bytes_fin[64..123].try_into().unwrap(),
                mark: bytes_fin[123],
            },
            mark: f32::from_le_bytes(bytes_fin[124..128].try_into().unwrap()),
        };

        result.push(Student::from_u8(s));
    }

    let result = serde_json::json!(result).to_string();

    write(format!("{}.json", filename), result);
}

fn json_to_bin(filename: &String) {
    let inp = File::options().read(true).open(filename).unwrap();

    let mut reader = BufReader::new(inp);

    let mut json_string = String::new();

    match reader.read_to_string(&mut json_string) {
        Ok(_) => {}
        Err(_) => {
            panic!("Failed to read JSON file");
        }
    }

    let students: Vec<Student> = serde_json::from_str(&json_string).unwrap();

    let mut c_structs = Vec::<u8>::new();
    for s in students {
        c_structs.extend_from_slice(s.name.as_bytes());
        c_structs.extend_from_slice(s.login.as_bytes());
        c_structs.extend_from_slice(s.group.as_bytes());
        c_structs.extend_from_slice(&s.practice);
        c_structs.extend_from_slice(s.project.repo.as_bytes());
        c_structs.push(s.project.mark);
        c_structs.extend_from_slice(&s.mark.to_le_bytes());
    }

    write(format!("{}.bin", filename), c_structs);
}

fn main() {
    let argv: Vec<String> = env::args().collect();

    if argv.len() < 2 {
        panic!("Cannot find path to file");
    }

    if argv[1].ends_with(".json") {
        json_to_bin(&argv[1]);
    } else {
        bin_to_json(&argv[1])
    }
}
