use std::io::{self, Read, Error, ErrorKind};
use std::fs::write;
use std::fs::File;
use std::mem;
use std::slice;
use serde::{Serialize, Deserialize};

#[repr(C, packed)]
#[derive(Debug, Copy, Clone, Default)]
struct CStudent {
    name: [u8; 32],
    login: [u8; 16],
    group: [u8; 8],
    practice: [u8; 8],
    project: CProject,
    mark: f32,
}

#[repr(C, packed)]
#[derive(Debug, Copy, Clone)]
struct CProject {
    repo: [u8; 59],
    mark: u8
}

fn to_bin(stud: &CStudent) -> Vec<u8> {
    let mut res = Vec::<u8>::new();
    res.extend_from_slice(&stud.name);
    res.extend_from_slice(&stud.login);
    res.extend_from_slice(&stud.group);
    res.extend_from_slice(&stud.practice);
    res.extend_from_slice(&stud.project.repo);
    res.push(stud.project.mark);
    res.extend_from_slice(&stud.mark.to_le_bytes());
    res
}

impl Default for CProject {
    fn default() -> Self {
        CProject { repo: [0; 59], mark: 0 }
    }
}

// [dkshtakin] use Result instead of Option,
fn from_c_string(bytes: &[u8]) -> Result<String, Error> {
    let mut idx: Option<usize> = None;
    let bytes_without_null = match bytes.iter().position(|&b| b == 0) {
        Some(ix) => {
            idx = Some(ix);
            &bytes[..ix]
        },
        None => bytes,
    };
    match idx {
        Some(x) => {
            match bytes[x..].iter().position(|&b| b != 0) {
                Some(_) => {
                    return Err(io::Error::new(io::ErrorKind::InvalidData, format!("invalid trailing zeros sequence")))
                },
                None => (),
            };
        },
        None => (),
    };

    // [dkshtakin] add quotes because quick_xml trims trailing and leading whitespaces
    return match std::str::from_utf8(bytes_without_null) {
        Ok(str) => Ok(format!("\"{}\"", str)),
        Err(e) => Err(io::Error::new(ErrorKind::Other, e))
    };
}

fn to_c_string(s: &String, bytes: & mut [u8]) {
    let sbytes = s.as_bytes();
    // [dkshtakin] trim quotes
    for i in 1..sbytes.len() - 1 {
        bytes[i - 1] = sbytes[i];
    }
    for i in sbytes.len() - 2..bytes.len() {
        bytes[i] = 0;
    }
}


#[derive(Debug, Serialize, Deserialize, Default)]
struct Student {
    name: String,
    login: String,
    group: String,
    practice: Vec<u8>,
    project: Project,
    mark: f32,
}

#[derive(Debug, Serialize, Deserialize, Default)]
struct Students {
    pub student: Vec<Student>,
}

#[derive(Debug, Serialize, Deserialize, Default)]
struct Root {
    pub students: Students,
}


#[derive(Debug, Serialize, Deserialize, Default)]
struct Project {
    repo: String,
    mark: u8,
}

impl Student {
    pub fn to_c(student: &Student) -> CStudent {
        let mut s = CStudent::default();
        to_c_string(&student.name, & mut s.name);
        to_c_string(&student.login, & mut s.login);
        to_c_string(&student.group, & mut s.group);
        for i in 0..student.practice.len() {
            s.practice[i] = student.practice[i]
        }
        to_c_string(&student.project.repo, & mut s.project.repo);
        s.project.mark = student.project.mark;
        s.mark = student.mark;
        s
    }
    pub fn from_c(student: &CStudent) -> Result<Student, Error> {
        // [dkshtakin] use Result instead of Option, unwrap on None calls panic
        let mut s = Student::default();
        s.name = from_c_string(&student.name)?;
        s.login = from_c_string(&student.login)?;
        s.group = from_c_string(&student.group)?;
        s.practice.resize(8, 0);
        for i in 0..student.practice.len() {
            // [dkshtakin] validate bool
            if student.practice[i] != 0 && student.practice[i] != 1 {
                return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid practice flag")));
            }
            s.practice[i] = student.practice[i]
        }
        s.project.repo = from_c_string(&student.project.repo)?;
        // [dkshtakin] validate project mark
        if student.project.mark > 10 {
            return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid project mark")));
        }
        s.project.mark = student.project.mark;
        // [dkshtakin] validate mark
        if student.mark < 0.0 || student.mark > 10.0 {
            return Err(io::Error::new(io::ErrorKind::InvalidData,format!("invalid student mark")));
        }
        // [dkshtakin] check float for NaN
        if student.mark.is_nan() {
            return Err(io::Error::new(io::ErrorKind::InvalidData,format!("student mark is nan")));
        }
        s.mark = student.mark;
        return Ok(s);
    }
}

// [dkshtakin] eof handling
fn eof_read_exact(f: &mut File, mut buf: &mut [u8]) -> Result<(), Error> {
    let mut has_content = false;
    while !buf.is_empty() {
        match f.read(buf) {
            Ok(0) => break,
            Ok(n) => {
                has_content = true;
                buf = &mut buf[n..];
            }
            Err(e) => return Err(e),
        }
    }
    if !buf.is_empty() && has_content {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("failed to fill whole buffer")));
    } else if !buf.is_empty() {
        return Err(io::Error::new(
            io::ErrorKind::UnexpectedEof,
            format!("eof")));
    } else {
        Ok(())
    }
}

pub fn formats_impl(filename: &mut String) -> Result<(), Error> {
    if filename.ends_with(".xml") {
        // println!("Detected xml file");
        let studxml = std::fs::read_to_string(filename.clone())?;
        // [dkshtakin] empty xml file
        if studxml.is_empty() {
            filename.truncate(filename.len() - 3);
            let outfile = format!("{}bin", filename);
            write(outfile, "")?;
            return Ok(());
        }
        // [dkshtakin] proper error handling
        let studs : Root = match quick_xml::de::from_str(&studxml) {
            Ok(content) => content,
            Err(e) => return Err(io::Error::new(ErrorKind::Other, e))
        };
        let mut result : Vec<u8> = vec![];
        for stud in studs.students.student {
            let studc = Student::to_c(&stud);
            let studbin : Vec<u8> = to_bin(&studc);
            result.extend(studbin);
        }
        filename.truncate(filename.len() - 3);
        let outfile = format!("{}bin", filename);
        write(outfile, result)?;
    } else if filename.ends_with(".bin") {
        // println!("Detected binary file");
        let mut file = File::open(filename.clone())?;
        let mut result =  Root { students: Students {
            student: vec![],
        }};
        let mut empty_file = true;
        loop {
            let mut cstud: CStudent = unsafe { mem::zeroed() };
            let cstud_size = mem::size_of::<CStudent>();
            unsafe {
                let cstud_slice = slice::from_raw_parts_mut(&mut cstud as *mut _ as *mut u8, cstud_size);
                match eof_read_exact(&mut file, cstud_slice) {
                    Ok(_) => {
                        empty_file = false;
                    },
                    Err(ref err) if err.kind() == io::ErrorKind::UnexpectedEof => {
                        // End of file reached
                        break;
                    },
                    Err(err) => {
                        // [dkshtakin] return error, do not crash
                        return Err(err);
                    },
                }
            }
            let stud = Student::from_c(&cstud)?;
            result.students.student.push(stud);
        }
        // [dkshtakin] proper error handling
        let studxml = match empty_file {
            true => "".to_string(),
            false => match quick_xml::se::to_string(&(result)) {
                Ok(str) => {str},
                Err(e) => return Err(io::Error::new(ErrorKind::Other, e))
            }
        };
        filename.truncate(filename.len() - 3);
        let outfile = format!("{}xml", filename);
        write(outfile, studxml)?;
    } else {
        // [dkshtakin] check file extension
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("wrong file extension ({})", filename)));
    }
    Ok(())
}
