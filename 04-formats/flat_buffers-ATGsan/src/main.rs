use std::env;
use std::fs::{File, write};
use std::io::Read;
use std::mem::size_of;
use serde::{Deserialize, Serialize};
use std::str;

mod student_generated;
use student_generated::decentralized::atgsan::{self};

const SIZE_OF_NAME: usize = 32usize;
const SIZE_OF_LOGIN: usize = 16usize;
const SIZE_OF_GROUP: usize = 8usize;
const SIZE_OF_PRACTICE: usize = 8usize;
const SIZE_OF_PROJECT_REPO: usize = 59usize;
const SIZE_OF_PROJECT_MARK: usize = 1usize;
const SIZE_OF_MARK: usize = size_of::<f32>();

const PADDING_OF_NAME: usize = 0;
const PADDING_OF_LOGIN: usize = PADDING_OF_NAME + SIZE_OF_NAME;
const PADDING_OF_GROUP: usize = PADDING_OF_LOGIN + SIZE_OF_LOGIN;
const PADDING_OF_PRACTICE: usize = PADDING_OF_GROUP + SIZE_OF_GROUP;
const PADDING_OF_PROJECT_REPO: usize = PADDING_OF_PRACTICE + SIZE_OF_PRACTICE;
const PADDING_OF_PROJECT_MARK: usize = PADDING_OF_PROJECT_REPO + SIZE_OF_PROJECT_REPO;
const PADDING_OF_MARK: usize = PADDING_OF_PROJECT_MARK + SIZE_OF_PROJECT_MARK;

#[derive(Serialize, Deserialize)]
struct Project {
    // URL
    repo: String,
    mark: u8,
}
#[derive(Serialize, Deserialize)]
struct Student {
    // имя может быть и короче 32 байт, тогда в хвосте 000
    // имя - валидный UTF-8
    name: String,
    // ASCII [\w]+
    login: String,
    group: String,
    // 0/1, фактически bool
    practice: String,
    project: Project,
    // 32 bit IEEE 754 float
    mark: f32,
}

const SIZE_OF_STUDENT: usize = size_of::<Student>() - 8;

fn read_file(file_name: &str) -> Vec<u8> {
    let file = File::open(&file_name);
    if file.is_err() {
        panic!(
            "It is impossible to open file {file_name}, reason: {:?}",
            file.err()
        );
    }
    let file = file.unwrap();
    let mut buf: Vec<u8> = vec![];
    if (&file).read_to_end(&mut buf).is_err() {
        panic!("Could not read data from file {file_name}");
    }
    buf
}

fn serialize(file_name: &String) {
    let data = read_file(&file_name);
    let mut serialized: Vec<atgsan::Student> = vec![];
    for i in 0..(data.len() / SIZE_OF_STUDENT) {
        let sl = &data[i*  SIZE_OF_STUDENT..(i + 1) * SIZE_OF_STUDENT];
        let flat_student: atgsan::Student = atgsan::Student(sl.try_into().unwrap());
        serialized.push(flat_student);
    }

    let mut builder = flatbuffers::FlatBufferBuilder::new();
    for i in serialized {
        builder.push(i);
    }
    write(format!("{}.flat", file_name), builder.finished_data()).expect("Error: could not write to output");
}

fn deserialize(file_name: &str) {
    let data = read_file(&file_name);
    for i in 0..(data.len() / SIZE_OF_STUDENT) {
        let sl = &data[i*  SIZE_OF_STUDENT..(i + 1) * SIZE_OF_STUDENT];
        let student = Student {
            name: String::from_utf8_lossy(&sl[0..SIZE_OF_NAME]).to_string(),
            login: String::from_utf8_lossy(&sl[PADDING_OF_LOGIN..PADDING_OF_GROUP]).to_string(),
            group: String::from_utf8_lossy(&sl[PADDING_OF_GROUP..PADDING_OF_PRACTICE]).to_string(),
            practice: String::from_utf8_lossy(&sl[PADDING_OF_PRACTICE..PADDING_OF_PROJECT_REPO]).to_string(),
            project: Project {
                repo: String::from_utf8_lossy(&sl[PADDING_OF_PROJECT_REPO..PADDING_OF_PROJECT_MARK]).to_string(),
                mark: sl[PADDING_OF_PROJECT_MARK],
            },
            mark: f32::from_le_bytes(sl[PADDING_OF_MARK..PADDING_OF_MARK + SIZE_OF_MARK].try_into().unwrap()),
        };
        write(format!("{}.bin", file_name), bincode::serialize(&student).unwrap()).expect("Error: could not write to output");
    }
}

pub fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        panic!("Not enough args");
    }

    if args[1].ends_with(".bin") {
        println!("SERIALIZE");
        serialize(&args[1]);
    } else if args[1].ends_with(".flat") {
        println!("DESERIALIZE");
        deserialize(&args[1]);
    } else {
        println!("Wrong format of file. Please put '*.bin' if you want to serialize; or put '*.flat' if you want to deserialize.");
    }
}
