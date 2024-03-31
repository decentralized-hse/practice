use crate::binformat::{Students, Student, Project};
use std::io;
use std::io::Cursor;

pub mod binformat {
    tonic::include_proto!("binformat");
}

// Rewritten this function, fixed wrong logic:
// The solution read bytes till the first error and returned Students, that were successfully parsed before error.
// This logic is wrong, because it doesn't provide round-trip guarantee. If the input is invalid, we should always return error
pub fn read_students_from_bin_file(f: &mut impl io::Read) -> Result<Students, Box<dyn std::error::Error>> {
    // println!("[*] Reading binary student data...");
    let mut buf = Vec::new();
    f.read_to_end(&mut buf).unwrap();
    let mut cursor = Cursor::new(buf);

    let mut students = Students::default();
    while (cursor.position() as usize) < cursor.get_ref().len() {
        let student = read_student_from_bin_file(&mut cursor)?;
        students.student.push(student);
    }
    Ok(students)
}

// changed the type of return error, because this function panicked, when got utf8 error and tried to unwrap it
fn read_student_from_bin_file(mut f: impl io::Read) -> Result<Student, Box<dyn std::error::Error>> {
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

    // changed 'unwrap()' to '?' to avoid panic
    Ok(Student{
        name: String::from_utf8(name)?,
        login: String::from_utf8(login)?,
        group: String::from_utf8(group)?,
        practice: practice,
        project: Some(Project{
            repo: String::from_utf8(project_repo)?,
            mark: u32::from(project_mark[0]),
        }),
        mark: f32::from_le_bytes(mark),
    })
}
    
pub fn write_students_to_bin_file(students: Students, f: &mut impl io::Write) {
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
        f.write_all(&project.mark.to_le_bytes()[0..1]).unwrap();
        f.write_all(&s.mark.to_le_bytes()).unwrap();
    }
}
