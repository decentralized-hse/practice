use std::io::{self, Read, Error};
use std::fs::write;
use std::fs::File;
use std::{mem, string};
use std::slice;
use serde::{Serialize, Deserialize};
use snailquote::{escape, unescape};


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

fn from_c_string(bytes: &[u8]) -> Option<&str> {
    let bytes_without_null = match bytes.iter().position(|&b| b == 0) {
        Some(ix) => &bytes[..ix],
        None => bytes,
    };

    std::str::from_utf8(bytes_without_null).ok()
}

fn to_c_string(s: &String, bytes: & mut [u8]) {
    let sbytes = s.as_bytes();
    for i in 0..sbytes.len() {
        bytes[i] = sbytes[i];
    }
    for i in sbytes.len()..bytes.len() {
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
    pub fn to_c(student: &Student) -> Result<CStudent, String> {
        let mut s = CStudent::default();
        to_c_string(&student.name, & mut s.name);
        to_c_string(&student.login, & mut s.login);
        to_c_string(&student.group, & mut s.group);
        for i in 0..student.practice.len() {
            let practice = student.practice[i];

            if practice == 0 || practice == 1 {
                s.practice[i] = student.practice[i]
            } else {
                return Err(format!("Can't use {practice} as practice element"))
            }
        }
        to_c_string(&student.project.repo, & mut s.project.repo);
        s.project.mark = student.project.mark;
        s.mark = student.mark;
        Ok(s)
    }
    pub fn from_c(student: &CStudent) -> Result<Student, String> {
        let mut s = Student::default();
        s.name = from_c_string(&student.name).unwrap().to_string();
        s.login = from_c_string(&student.login).unwrap().to_string();
        s.group = from_c_string(&student.group).unwrap().to_string();
        s.practice.resize(8, 0);
        for i in 0..student.practice.len() {
            let practice = student.practice[i];

            if practice == 0 || practice == 1 {
                s.practice[i] = student.practice[i]
            } else {
                return Err(format!("Can't use {practice} as practice element"))
            }
        }
        s.project.repo = from_c_string(&student.project.repo).unwrap().to_string();
        s.project.mark = student.project.mark;
        s.mark = student.mark;
        Ok(s)
    }
}

fn unescape_for_xml(string: &String) -> String{
    return quick_xml::escape::unescape(unescape(string).unwrap().as_str()).unwrap().to_string();
}

fn escape_for_xml(string: &String) -> String {
    return escape(quick_xml::escape::escape(string).to_string().as_str()).to_string();
}

fn main() -> Result<(), Error> {
    let mut argv: Vec<String> = std::env::args().collect();
    if argv.len() < 2{
        panic!("NO file provided!");
    }

    let filename = & mut argv[1];

    if filename.ends_with(".xml") {
        println!("Detected xml file");
        let studxml = std::fs::read_to_string(filename.clone())?;
        let studs : Root = quick_xml::de::from_str(&studxml).unwrap();
        let mut result : Vec<u8> = vec![];
        for mut stud in studs.students.student {            
            stud.name = unescape_for_xml(&stud.name);
            stud.project.repo = unescape_for_xml(&stud.project.repo);
            let studc = Student::to_c(&stud).unwrap();
            let studbin : Vec<u8> = to_bin(&studc);
            result.extend(studbin);
        }
        filename.truncate(filename.len() - 3);
        let outfile = format!("{}bin", filename);
        write(outfile, result)?;
    } else {
        println!("Detected binary file");
        let mut file = File::open(filename.clone())?;
        let mut result =  Root { students: Students {
            student: vec![],
        }};
        loop {
            let mut cstud: CStudent = unsafe { mem::zeroed() };
            let cstud_size = mem::size_of::<CStudent>();
            unsafe {
                let cstud_slice = slice::from_raw_parts_mut(&mut cstud as *mut _ as *mut u8, cstud_size);
                match file.read_exact(cstud_slice) {
                    Ok(_) => {},
                    Err(ref err) if err.kind() == io::ErrorKind::UnexpectedEof => {
                        // End of file reached
                        break;
                    },
                    Err(err) => {
                        panic!("Failed reading file: {}", err);
                    },
                }
            }
            let mut stud = Student::from_c(&cstud).unwrap();
            stud.name = escape_for_xml(&stud.name);
            stud.project.repo = escape_for_xml(&stud.project.repo);
            result.students.student.push(stud);
        }
        let studxml = quick_xml::se::to_string(&(result)).unwrap();
        filename.truncate(filename.len() - 3);
        let outfile = format!("{}xml", filename);
        write(outfile, studxml)?;
    }
    Ok(())
}
