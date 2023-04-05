import os
import flatbuffers
from schema import Project, Root, Student
import struct
import sys

student_format = '<32s16s8s8B59sBf'

def read_flat(file):
    buf = file.read()
    students = []

    root = Root.Root.GetRootAsRoot(buf, 0)
    students_len = root.StudentsLength()
    for i in range(students_len):
        student_flat = Student.Student()
        if not root.Students(i):
            raise ValueError(f"no {i} student")

        student = Student.Student()
        student.Name = student_flat.Name().decode('utf-8')
        student.Login = student_flat.Login().decode('utf-8')
        student.Group = student_flat.Group().decode('utf-8')

        practice_len = student_flat.PracticeLength()
        for j in range(practice_len):
            student.practice[j] = student_flat.Practice(j)

        Project = Project()
        if student_flat.Project(Project) is None:
            raise ValueError(f"no {i} student's Project")

        student.Project.repo = Project.Repo().decode('utf-8')
        student.Project.Mark = Project.Mark()

        student.Mark = student_flat.Mark()

        students.append(student)
    return students

def write_flat(file, students : list[Student.Student]):
    builder = flatbuffers.Builder(2048)
    students_tables = []

    for st in students:
        if type(st) != Student:
            print(st)
            continue
        Name = builder.CreateString(btos(st.Name))
        Login = builder.CreateString(btos(st.Login))
        Group = builder.CreateString(btos(st.Group))

        Student.StudentStartPracticeVector(builder, 8)

        for i in range(len(st.practice)):
            builder.PrependByte(st.practice[len(st.practice)-1-i])

        practice = builder.EndVector(8)
        repo = builder.CreateString(btos(st.Project.repo))

        Project.ProjectStart(builder)
        Project.ProjectAddRepo(builder, repo)
        Project.ProjectAddMark(builder, st.Project.Mark)

        Project = Project.ProjectEnd(builder)
        Project.StudentStart(builder)
        Project.StudentAddName(builder, Name)
        Project.StudentAddLogin(builder, Login)
        Project.StudentAddGroup(builder, Group)
        Project.StudentAddPractice(builder, practice)
        Project.StudentAddProject(builder, Project)
        Project.StudentAddMark(builder, st.Mark)
        students_tables.append(Student.StudentEnd(builder))

    Root.RootStartStudentsVector(builder, len(students))
    for i in range(len(students_tables)):
        builder.PrependUOffsetT(students_tables[len(students_tables)-1-i])

    students_vec = builder.EndVector(len(students))

    Root.RootStart(builder)
    Root.RootAddStudents(builder, students_vec)
    root = Root.RootEnd(builder)

    builder.Finish(root)
    buf = builder.Output()
    file.write(buf)

def read_bin(file):
    students = []
    while True:
        student_data = file.read(struct.calcsize(student_format))
        if not student_data:
            return students
        student = Student.Student.GetRootAs(student_data)
        students.append(student)

def write_bin(file, students):
    for student in students:
        student_data = struct.pack(student_format, *student)
        file.write(student_data)

def btos(s):
    i = s.index(0)
    if i != -1:
        return s[:i].decode('utf-8')
    return s.decode('utf-8')

def output(path):
    file = open(path, 'wb')
    return file

def main():
    input_file = open(sys.argv[1], 'rb')
    input_file_extension = os.path.splitext(sys.argv[1])[1]
    if input_file_extension == '.bin':
        student = read_bin(input_file)
        output_path = os.path.splitext(sys.argv[1])[0] + '.flat'
        output_file = output(output_path)
        write_flat(output_file, student)
    elif input_file_extension == '.flat':
        student = read_flat(input_file)
        output_path = os.path.splitext(sys.argv[1])[0] + '.bin'
        print(output_path)
        output_file = output(output_path)
        write_bin(output_file, student)
    else:
        print('Invalid file extension')
    input_file.close()

main()