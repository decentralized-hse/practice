use rusqlite::params;

#[derive(Debug, PartialEq)]
struct Project {
    repo: [u8; 59],
    mark: u8,
}

#[derive(Debug, PartialEq)]
struct Student {
    name: [u8; 32],
    login: [u8; 16],
    group: [u8; 8],
    practice: [u8; 8],
    project: Project,
    mark: f32,
}

impl Student {
    fn load_from_file(mut f: impl std::io::Read) -> std::io::Result<Self> {
        let mut res = Student {
            name: [0; 32],
            login: [0; 16],
            group: [0; 8],
            practice: [0; 8],
            project: Project {
                repo: [0; 59],
                mark: 0,
            },
            mark: 0.0,
        };

        f.read_exact(&mut res.name)?;
        f.read_exact(&mut res.login)?;
        f.read_exact(&mut res.group)?;
        f.read_exact(&mut res.practice)?;
        f.read_exact(&mut res.project.repo)?;
        {
            let mut buf = [0u8; std::mem::size_of::<u8>()];
            f.read_exact(&mut buf)?;
            res.project.mark = u8::from_le_bytes(buf);
        }
        {
            let mut buf = [0u8; std::mem::size_of::<f32>()];
            f.read_exact(&mut buf)?;
            res.mark = f32::from_le_bytes(buf);
        }

        Ok(res)
    }

    fn save_to_file(&self, mut f: impl std::io::Write) -> std::io::Result<()> {
        f.write_all(&self.name)?;
        f.write_all(&self.login)?;
        f.write_all(&self.group)?;
        f.write_all(&self.practice)?;
        f.write_all(&self.project.repo)?;
        f.write_all(&self.project.mark.to_le_bytes())?;
        f.write_all(&self.mark.to_le_bytes())?;

        Ok(())
    }

    fn save_to_db(&self, conn: &rusqlite::Connection) -> rusqlite::Result<()> {
        conn.execute(
            "insert into student (name, login, student_group, practice, project_repo, project_mark, mark)
                  values (?1, ?2, ?3, ?4, ?5, ?6, ?7)",
            params![
                String::from_utf8(self.name.to_vec()).unwrap(),
                String::from_utf8(self.login.to_vec()).unwrap(),
                String::from_utf8(self.group.to_vec()).unwrap(),
                Vec::from(self.practice),
                String::from_utf8(self.project.repo.to_vec()).unwrap(),
                self.project.mark,
                self.mark,
            ],
        )?;

        Ok(())
    }
}

fn load_students_from_db(conn: &rusqlite::Connection) -> rusqlite::Result<Vec<Student>> {
    let mut stmt = conn.prepare("select name, login, student_group, practice, project_repo, project_mark, mark from student")?;
    let student_iter = stmt.query_map([], |row| {
        Ok(Student {
            name: row.get::<usize, String>(0)?.as_bytes().try_into().unwrap(),
            login: row.get::<usize, String>(1)?.as_bytes().try_into().unwrap(),
            group: row.get::<usize, String>(2)?.as_bytes().try_into().unwrap(),
            practice: row.get::<usize, Vec<u8>>(3)?.as_slice().try_into().unwrap(),
            project: Project {
                repo: row.get::<usize, String>(4)?.as_bytes().try_into().unwrap(),
                mark: row.get(5)?,
            },
            mark: row.get(6)?,
        })
    })?;

    let mut students = Vec::new();
    for student in student_iter {
        students.push(student?);
    }

    Ok(students)
}

fn main() {
    let filename = std::env::args().nth(1).unwrap();
    let prefix = filename.rsplit_once('.').unwrap().0.to_string();
    if filename.ends_with(".bin") {
        println!("Reading binary student data from {}...", filename);
        let mut f = std::fs::File::open(filename).unwrap();
        let mut students = Vec::new();
        while let Ok(student) = Student::load_from_file(&mut f) {
            students.push(student);
        }
        println!("{} students read...", students.len());

        let db_filename = prefix + ".sqlite";
        let conn = rusqlite::Connection::open(db_filename.clone()).unwrap();
        conn.execute("drop table if exists student", []).unwrap();
        conn.execute(
            "create table student (
                 name text not null,
                 login text not null,
                 student_group text not null,
                 practice blob not null,
                 project_repo text not null,
                 project_mark integer not null,
                 mark real not null
         )",
            [],
        ).unwrap();
        for student in students {
            student.save_to_db(&conn).unwrap();
        }
        println!("written to {}", db_filename);
    } else if filename.ends_with(".sqlite") {
        println!("Reading sqlite student data from {}...", filename);
        let conn = rusqlite::Connection::open(filename).unwrap();
        let students = load_students_from_db(&conn).unwrap();
        println!("{} students read...", students.len());

        let bin_filename = prefix + ".bin";
        let mut f = std::fs::File::create(bin_filename.clone()).unwrap();
        for student in students {
            student.save_to_file(&mut f).unwrap();
        }
        println!("written to {}", bin_filename);
    } else {
        panic!("Unknown file type");
    }
}
