use std::io::{self, prelude::*, BufReader};
use std::fs::File;
use iowrap::Eof;
use std::str::from_utf8;
use text_io::scan;
use serde::{Deserialize, Serialize};
use serde_big_array::BigArray;

fn str_as_array<const SIZE: usize>(s: &str) -> [u8; SIZE] {
    let mut array: [u8; SIZE] = [0; SIZE];
    let size = std::cmp::min(SIZE, s.len());
    let str_bytes = s.as_bytes();
    for i in 0..size {
        array[i] = str_bytes[i]
    }
    array
}

#[derive(Debug, Serialize, Deserialize)]
struct Project {
    #[serde(with = "BigArray")]
    repo: [u8; 59],
    mark: u8,
}

#[derive(Debug, Serialize, Deserialize)]
struct Student {
    name: [u8; 32],
    login: [u8; 16],
    group: [u8; 8],
    practice: [u8; 8],
    project: Project,
    mark: f32,
}

fn from_bin_to_kv(filepath: &str) -> io::Result<()> {
    println!("Reading binary student data from {}...", filepath);

    let mut f = Eof::new(File::open(filepath)?);
    let mut buffer = [0; 128];
    let mut students = 0;

    let new_filepath = format!("{}.kv", filepath[..filepath.len() - 4].to_owned());
    let mut f_new = File::create(&new_filepath)?;

    while !f.eof()? {
        let _ = f.read_exact(&mut buffer)?;
        let deserialized: Student = bincode::deserialize(&buffer).unwrap();

        writeln!(f_new, "[{}].name = {}", students, from_utf8(&deserialized.name).unwrap())?;
        writeln!(f_new, "[{}].login = {}", students, from_utf8(&deserialized.login).unwrap())?;
        writeln!(f_new, "[{}].group = {}", students, from_utf8(&deserialized.group).unwrap())?;
        writeln!(f_new, "[{}].practice = {:?}", students, deserialized.practice)?;
        writeln!(f_new, "[{}].project.repo = {}", students, from_utf8(&deserialized.project.repo).unwrap())?;
        writeln!(f_new, "[{}].project.mark = {}", students, deserialized.project.mark)?;
        writeln!(f_new, "[{}].mark = {}", students, deserialized.mark)?;

        students += 1;   
    }

    println!("{} students read...", students);
    println!("written to {}", &new_filepath);
    Ok(())
}

fn from_kv_to_bin(filepath: &str) -> io::Result<()> {
    println!("Reading key-value student data from {}...", filepath);

    let mut f = BufReader::new(File::open(filepath)?);
    let mut students = 0;

    let new_filepath = format!("{}.bin", filepath[..filepath.len() - 3].to_owned());
    let mut f_new = File::create(&new_filepath)?;

    loop {
        let mut student_id: i32;

        let mut name_string = String::new();
        let mut name = String::new();
        let num_bytes = f.read_line(&mut name_string)?;
        if num_bytes == 0 {
            break;
        }
        scan!(name_string.bytes() => "[{}].name = {}", student_id, name);

        let mut login_string = String::new();
        let mut login = String::new();
        let _ = f.read_line(&mut login_string)?;
        scan!(login_string.bytes() => "[{}].login = {}", student_id, login);

        let mut group_string = String::new();
        let mut group = String::new();
        let _ = f.read_line(&mut group_string)?;
        scan!(group_string.bytes() => "[{}].group = {}", student_id, group);

        let mut practice_string = String::new();
        let mut practice = String::new();
        let _ = f.read_line(&mut practice_string)?;
        scan!(practice_string.bytes() => "[{}].practice = [{}]", student_id, practice);
        let practice: Vec::<u8> = practice
            .split(", ")
            .map(|n| n.parse().unwrap())
            .collect();
        let practice: [u8; 8] = practice.try_into().unwrap();

        let mut project_repo_string = String::new();
        let mut project_repo = String::new();
        let _ = f.read_line(&mut project_repo_string)?;
        scan!(project_repo_string.bytes() => "[{}].project.repo = {}", student_id, project_repo);

        let mut project_mark_string = String::new();
        let mut project_mark = String::new();
        let _ = f.read_line(&mut project_mark_string)?;
        scan!(project_mark_string.bytes() => "[{}].project.mark = {}", student_id, project_mark);

        let mut mark_string = String::new();
        let mut mark = String::new();
        let _ = f.read_line(&mut mark_string)?;
        scan!(mark_string.bytes() => "[{}].mark = {}", student_id, mark);

        let s = Student {
            name: str_as_array(&name),
            login: str_as_array(&login),
            group: str_as_array(&group),
            practice: practice,
            project: Project {
                repo: str_as_array(&project_repo),
                mark: project_mark.parse().unwrap(),
            },
            mark: mark.parse().unwrap(),
        };
        
        bincode::serialize_into(&mut f_new, &s).unwrap();

        students += 1
    }


    println!("{} students read...", students);
    println!("written to {}", &new_filepath);
    Ok(())
}

fn main() -> io::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    assert!(args.len() == 2);

    if args[1].ends_with(".bin") {
        from_bin_to_kv(&args[1])?;
    } else if args[1].ends_with(".kv") {
        from_kv_to_bin(&args[1])?;
    } else {
        panic!("Unnown file type")
    }

    Ok(())




    /*
    let s = Student {
        name: str_as_array("Иван Иванов"),
        login: str_as_array("ivanov"),
        group: str_as_array("ИА-345"),
        practice: [1, 1, 0, 0, 0, 0, 0, 0],
        project: Project {
            repo: str_as_array("github.com/decentralized-hse/practice/tree/main/04-formats"),
            mark: 9,
        },
        mark: 8.0,
    };

    println!("{:?}", s);
    println!("{}", std::str::from_utf8(&s.name).unwrap());

    {
        let mut f = File::create("./output.bin").unwrap();
        bincode::serialize_into(&mut f, &s).unwrap();
    }
    {
        let mut buffer = [0; 128];
        let mut f = File::open("./output.bin").unwrap();
        f.read_exact(&mut buffer)?;

        let deserialized: Student = bincode::deserialize(&buffer).unwrap();
        println!("deserialized = {:?}", deserialized);
    }
    Ok(())
    */
}
