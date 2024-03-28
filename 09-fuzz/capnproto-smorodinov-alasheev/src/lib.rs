use std::cmp::min;

use std::process::exit;
use std::vec;

use capnp::message::Builder;
use capnp::message::ReaderOptions;

use capnp::serialize;

pub mod students_capnp;

pub const NAME_LEN: usize = 32;
pub const LOGIN_LEN: usize = 16;
pub const GROUP_LEN: usize = 8;
pub const PRACTICE_LEN: usize = 8;
pub const PROJECT_REPO_LEN: usize = 59;
pub const PROJECT_MARK_LEN: usize = 1;
pub const MARK_LEN: usize = 4;

pub const TOTAL_LEN: usize = NAME_LEN
    + LOGIN_LEN
    + GROUP_LEN
    + PRACTICE_LEN
    + PROJECT_REPO_LEN
    + PROJECT_MARK_LEN
    + MARK_LEN;


fn write_bytes(offset: &mut usize, dst: &mut [u8], src: &[u8], length: usize) {
    let len = min(length, src.len());

    let from = *offset;
    let to = from + len;

    dst[from..to].copy_from_slice(&src);

    *offset += length;
}

fn string_from_bytes(bytes: &[u8]) -> String {
    String::from_utf8(bytes.to_vec()).unwrap().to_string()
}

fn safe_string_from_bytes(bytes: &[u8]) -> Result<String, &'static str> {
    match String::from_utf8(bytes.to_vec()) {
        Ok(s) => Ok(s),
        Err(_) => Err("bad"),
    }
}

fn read_bytes<'a>(bytes: &'a [u8], offset: &mut usize, length: usize) -> &'a [u8] {
    let from = *offset;
    let to = from + length;
    *offset += length;

    return &bytes[from..to];
}


fn try_to_read(input: &[u8]) -> Result<(), &'static str> {
    let count = input.len() / TOTAL_LEN;

    for (_i, input_offset) in (0..count * TOTAL_LEN).step_by(TOTAL_LEN).enumerate() {
        let bytes = &input[input_offset..input_offset + TOTAL_LEN];
        let mut offset = 0;

        safe_string_from_bytes(read_bytes(bytes, &mut offset, NAME_LEN))?;
        safe_string_from_bytes(read_bytes(bytes, &mut offset, LOGIN_LEN))?;
        safe_string_from_bytes(read_bytes(bytes, &mut offset, GROUP_LEN))?;
        read_bytes(bytes, &mut offset, PRACTICE_LEN).to_vec();
        safe_string_from_bytes(read_bytes(bytes, &mut offset, PROJECT_REPO_LEN))?;
        read_bytes(bytes, &mut offset, PROJECT_MARK_LEN)[0];
        match TryInto::<[u8; 4]>::try_into(read_bytes(bytes, &mut offset, MARK_LEN)) {
            Ok(_) => (),
            Err(_) => return Err("bad"),
        };
    }
    Ok(())
}

pub fn validate_input(input: &[u8]) -> bool {
    if input.len() % TOTAL_LEN != 0 {
        return false;
    }

    try_to_read(input).is_ok()
}

pub fn bin_to_capnproto(input: &[u8]) -> Vec<u8> {
    if !validate_input(input) {
        println!("Invalid input");
        exit(0);
    }
    assert!(
        input.len() % TOTAL_LEN == 0,
        "Corrupt file, length is not divisible by size of one Student"
    );

    let count = input.len() / TOTAL_LEN;

    let mut students_message = Builder::new_default();
    let mut students_root = students_message.init_root::<students_capnp::students::Builder>();
    let mut students_builder = students_root.reborrow().init_students(count as u32);

    for (i, input_offset) in (0..count * TOTAL_LEN).step_by(TOTAL_LEN).enumerate() {
        let bytes = &input[input_offset..input_offset + TOTAL_LEN];
        let mut offset = 0;

        let name = string_from_bytes(read_bytes(bytes, &mut offset, NAME_LEN));
        let login = string_from_bytes(read_bytes(bytes, &mut offset, LOGIN_LEN));
        let group = string_from_bytes(read_bytes(bytes, &mut offset, GROUP_LEN));
        let practice = read_bytes(bytes, &mut offset, PRACTICE_LEN).to_vec();
        let project_repo = string_from_bytes(read_bytes(bytes, &mut offset, PROJECT_REPO_LEN));
        let project_mark = read_bytes(bytes, &mut offset, PROJECT_MARK_LEN)[0];
        let mark = f32::from_le_bytes(read_bytes(bytes, &mut offset, MARK_LEN).try_into().unwrap());

        // build student

        let mut student = students_builder.reborrow().get(i as u32);
        student.set_name(&name);
        student.set_login(&login);
        student.set_group(&group);

        let mut practice_builder = student.reborrow().init_practice(practice.len() as u32);
        for (j, &value) in practice.iter().enumerate() {
            practice_builder.reborrow().set(j as u32, value);
        }

        let mut project = student.reborrow().get_project().unwrap();
        project.set_repo(&project_repo);
        project.set_mark(project_mark);

        student.set_mark(mark);
    }

    serialize::write_message_to_words(&students_message)
}

pub fn capnproto_to_bin(input: &[u8]) -> Vec<u8> {
    let reader = serialize::read_message(input, ReaderOptions::new()).unwrap();
    let students = reader
        .get_root::<students_capnp::students::Reader>()
        .unwrap();

    let mut res = Vec::<u8>::new();

    for student in students.get_students().unwrap().iter() {
        let mut bytes = vec![0u8; TOTAL_LEN];
        let mut offset = 0;

        let name = student.get_name().unwrap().as_bytes();
        let login = student.get_login().unwrap().as_bytes();
        let group = student.get_group().unwrap().as_bytes();

        let practice = student.get_practice().unwrap();
        let practice_vec = practice.into_iter().collect::<Vec<u8>>();
        let practice_slice = practice_vec.as_slice();

        let project = student.get_project().unwrap();
        let project_repo = project.get_repo().unwrap().as_bytes();
        let project_mark = &[project.get_mark()];

        // Cap'n Proto encodes f32 as little-endian 32 bit IEEE 754 float
        let mark = &student.get_mark().to_le_bytes();

        write_bytes(&mut offset, &mut bytes, name, NAME_LEN);
        write_bytes(&mut offset, &mut bytes, login, LOGIN_LEN);
        write_bytes(&mut offset, &mut bytes, group, GROUP_LEN);
        write_bytes(&mut offset, &mut bytes, practice_slice, PRACTICE_LEN);
        write_bytes(&mut offset, &mut bytes, project_repo, PROJECT_REPO_LEN);
        write_bytes(&mut offset, &mut bytes, project_mark, PROJECT_MARK_LEN);
        write_bytes(&mut offset, &mut bytes, mark, MARK_LEN);

        res.append(&mut bytes);
    }

    res
}
