use prost::{
    bytes::{BufMut, BytesMut},
    Message,
};
use protobuf_zhukov::binformat::{Students, Student, Project};
use core::panic;
use std::{env, fs, io, string::String};


const EXT_BIN: &str = ".bin";
const EXT_PROTO: &str = ".protobuf";

fn read_students_from_protobuf_file(f: &mut impl io::Read) -> Students {
    let mut b = BytesMut::new().writer();
    io::copy(f, &mut b).expect("[x] Can't read proto file");
    Students::decode(b.into_inner()).expect("[x] Can't decode proto file")
}

fn read_students_from_bin_file(f: &mut impl io::Read) -> Students {
    let mut students = Students::default();
    println!("[*] Reading binary student data...");
    while let Ok(student) = read_student_from_bin_file(&mut *f) {
        students.student.push(student);
    }
    students
}

fn read_student_from_bin_file(mut f: impl io::Read) -> Result<Student, io::Error> {
    let mut name = vec![0u8; 32];
    let mut login = vec![0u8; 16];
    let mut group = vec![0u8; 8];
    let mut practice = vec![0u8; 8];
    let mut project_repo = vec![0u8; 59];
    let mut project_mark = [0u8; 1];
    let mut mark = [0u8; 4];

    f.read_exact(&mut name)?;
    f.read_exact(&mut login)?;
    f.read_exact(&mut group)?;
    f.read_exact(&mut practice)?;
    f.read_exact(&mut project_repo)?;
    f.read_exact(&mut project_mark)?;
    f.read_exact(&mut mark)?;

    Ok(Student{
        name: String::from_utf8(name).unwrap(),
        login: String::from_utf8(login).unwrap(),
        group: String::from_utf8(group).unwrap(),
        practice: practice,
        project: Some(Project{
            repo: String::from_utf8(project_repo).unwrap(),
            mark: u32::from(project_mark[0]),
        }),
        mark: f32::from_le_bytes(mark),
    })
}


fn write_students_to_protobuf_file(students: Students, f: &mut impl io::Write) {
    let mut buf: Vec<u8> = Vec::with_capacity(300);
    students.encode(&mut buf).unwrap();
    f.write_all(&buf).expect("[x] Can't write to proto file")
}
    
fn write_students_to_bin_file(students: Students, f: &mut impl io::Write) {
    for s in students.student {
        f.write_all(s.name.as_bytes()).unwrap();
        f.write_all(s.login.as_bytes()).unwrap();
        f.write_all(s.group.as_bytes()).unwrap();
        f.write_all(&s.practice).unwrap();
        let project = match s.project {
            Some(val) => val,
            None => Project::default(),
        };
        f.write_all(project.repo.as_bytes()).unwrap();
        f.write_all(&project.mark.to_le_bytes()).unwrap();
        f.write_all(&s.mark.to_le_bytes()).unwrap();
    }
}


fn main() { 
    let argv: Vec<String> = env::args().collect();
    assert!(argv.len() == 2, "[x] Incorrect number of arguments");

    let input_filename = argv[1].clone();

    let mut ext_from = EXT_BIN.clone();
    let mut ext_to = EXT_PROTO.clone();
    if input_filename.ends_with(EXT_PROTO) {
        (ext_from, ext_to) = (ext_to, ext_from);
    } else if !input_filename.ends_with(EXT_BIN) {
        panic!("[x] Unexpected file extension");
    }

    let output_filename = input_filename.rsplit_once('.').unwrap().0.to_string() + ext_to;
    
    let mut ifile = fs::File::open(input_filename).expect("[x] Can't open file");
    let students: Students;
    if ext_from == EXT_BIN {
        students = read_students_from_bin_file(&mut ifile);
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
