use arrayvec::ArrayVec;
use serde::{Deserialize, Serialize};
use serde_json::to_string_pretty;
use std::fs::write;
use std::fs::File;
use std::io::{BufReader, Read};
use std::vec;

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

pub fn check_raw_bytes(bytes_fin: [u8; SIZE]) -> bool {
    return !check_utf8(&bytes_fin[0..32])
        || !check_utf8(&bytes_fin[32..48])
        || !check_utf8(&bytes_fin[48..56])
        || !check_utf8(&bytes_fin[64..123]);
}
pub fn check_utf8(s: &[u8]) -> bool {
    std::str::from_utf8(s).is_ok()
}

impl Student {
    pub fn is_valid_utf8(s: &str) -> bool {
        std::str::from_utf8(s.as_bytes()).is_ok()
    }

    pub fn from_u8(init: StudentU8) -> Result<Self, String> {
        let name = String::from_utf8_lossy(&init.name)
            .trim_end_matches('\0')
            .to_string();
        let login = String::from_utf8_lossy(&init.login)
            .trim_end_matches('\0')
            .to_string();
        let group = String::from_utf8_lossy(&init.group)
            .trim_end_matches('\0')
            .to_string();

        let project_repo = String::from_utf8_lossy(&init.project.repo)
            .trim_end_matches('\0')
            .to_string();

        if !Student::is_valid_utf8(name.as_str())
            || !Student::is_valid_utf8(login.as_str())
            || !Student::is_valid_utf8(group.as_str())
            || !Student::is_valid_utf8(project_repo.as_str())
        {
            return Err("Invalid UTF-8 data".to_string());
        }

        let project = Mark {
            repo: project_repo,
            mark: init.project.mark,
        };

        Ok(Student {
            name,
            login,
            group,
            practice: init.practice,
            project,
            mark: init.mark,
        })
    }
}

pub fn bin_to_json(filename: &mut String) -> Result<(), String> {
    let inp = match File::open(&filename) {
        Ok(file) => file,
        Err(error) => {
            return Err(format!("Failed to open binary file: {}", error).to_string());
        }
    };

    let mut reader = BufReader::new(inp);

    let mut bytes = Vec::new();

    match reader.read_to_end(&mut bytes) {
        Ok(_) => {}
        Err(err) => {
            return Err(format!("Failed to open binary file: {}", err).to_string());
        }
    }
    if bytes.len() % SIZE != 0 {
        return Err(format!("Invalid file").to_string());
    }

    let dst: Vec<&[u8]> = bytes.chunks(SIZE).collect();

    let mut result: Vec<Student> = vec![];

    for i in 0..dst.len() {
        let slice = dst[i];
        let array: ArrayVec<&u8, SIZE> = slice.into_iter().collect();
        let bytes: [&u8; SIZE] = array.into_inner().unwrap();

        let mut bytes_fin = [0u8; SIZE];
        for i in 0..bytes.len() {
            bytes_fin[i] = *bytes[i];
        }

        if check_raw_bytes(bytes_fin) {
            return Err("Invalid UTF-8 data".to_string());
        }

        let row_bytes = match bytes_fin[124..128].try_into() {
            Ok(mark) => mark,
            Err(_) => {
                return Err(format!("Failed to convert student name").to_string());
            }
        };
        let mark = f32::from_le_bytes(row_bytes);
        let s = StudentU8 {
            name: match bytes_fin[0..32].try_into() {
                Ok(name) => name,
                Err(_) => {
                    return Err(format!("Failed to convert student name").to_string());
                }
            },
            login: match bytes_fin[32..48].try_into() {
                Ok(login) => login,
                Err(_) => {
                    return Err(format!("Failed to convert student login").to_string());
                }
            },
            group: match bytes_fin[48..56].try_into() {
                Ok(group) => group,
                Err(_) => {
                    return Err(format!("Failed to convert student group").to_string());
                }
            },
            practice: match bytes_fin[56..64].try_into() {
                Ok(practice) => practice,
                Err(_) => {
                    return Err(format!("Failed to convert student practice").to_string());
                }
            },
            project: MarkU8 {
                repo: match bytes_fin[64..123].try_into() {
                    Ok(repo) => repo,
                    Err(_) => {
                        return Err(format!("Failed to convert project repo").to_string());
                    }
                },
                mark: bytes_fin[123],
            },
            mark,
        };

        match Student::from_u8(s) {
            Ok(student) => result.push(student),
            Err(error) => {
                return Err(format!("Failed to create student: {}", error).to_string());
            }
        }
    }

    let result_json = match to_string_pretty(&result) {
        Ok(json) => json,
        Err(error) => {
            return Err(format!("Failed to convert to JSON: {}", error).to_string());
        }
    };

    filename.truncate(filename.len() - 3);
    if let Err(error) = write(format!("{}json", filename), result_json) {
        return Err(format!("Failed to write JSON file: {}", error).to_string());
    }
    return Ok(());
}

pub fn json_to_bin(filename: &mut String) -> Result<(), String> {
    let inp = match File::open(&filename) {
        Ok(file) => file,
        Err(error) => {
            return Err(format!("Failed to open JSON file: {}", error).to_string());
        }
    };

    let mut reader = BufReader::new(inp);

    let mut json_string = String::new();

    if let Err(error) = reader.read_to_string(&mut json_string) {
        return Err(format!("Failed to read JSON file: {}", error).to_string());
    }

    let students: Vec<Student> = match serde_json::from_str(&json_string) {
        Ok(data) => data,
        Err(error) => {
            return Err(format!("Failed to parse JSON: {}", error).to_string());
        }
    };

    let mut c_structs = Vec::<u8>::new();
    for s in students {
        let mut cur_struct = Vec::<u8>::new();

        //////////////////////////////////////////////////////////////// Name
        cur_struct.extend_from_slice(s.name.as_bytes());
        if cur_struct.len() > 32 {
            return Err(format!("Invalid json format: name").to_string());
        }
        cur_struct.extend_from_slice(&vec![0u8; 32 - cur_struct.len()]);

        /////////////////////////////////////////////////////////////// Login
        cur_struct.extend_from_slice(s.login.as_bytes());
        if cur_struct.len() > 48 {
            return Err(format!("Invalid json format: login").to_string());
        }
        cur_struct.extend_from_slice(&vec![0u8; 48 - cur_struct.len()]);

        ////////////////////////////////////////////////////////////// Group
        cur_struct.extend_from_slice(s.group.as_bytes());
        if cur_struct.len() > 56 {
            return Err(format!("Invalid json format: group").to_string());
        }
        cur_struct.extend_from_slice(&vec![0u8; 56 - cur_struct.len()]);

        ////////////////////////////////////////////////////////////// practice
        cur_struct.extend_from_slice(&s.practice);

        ////////////////////////////////////////////////////////////// repo
        cur_struct.extend_from_slice(s.project.repo.as_bytes());
        if cur_struct.len() > 123 {
            return Err(format!("Invalid json format: prohect repo").to_string());
        }
        cur_struct.extend_from_slice(&vec![0u8; 123 - cur_struct.len()]);

        cur_struct.push(s.project.mark);
        cur_struct.extend_from_slice(&s.mark.to_le_bytes());
        c_structs.extend(cur_struct.iter());
    }
    filename.truncate(filename.len() - 4);
    if let Err(error) = write(format!("{}bin", filename), c_structs) {
        return Err(format!("Failed to write binary file: {}", error).to_string());
    }
    return Ok(());
}
