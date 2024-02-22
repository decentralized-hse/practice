import os
import flatbuffers
from .schema import Project, Root, Student
import struct
import sys
from io import FileIO

student_format = '<32s16s8s8B59sBf'

def flat_to_bin(input_file: FileIO, output_file: FileIO):
    buf = input_file.read()
    buf = bytearray(buf)
    root = Root.Root.GetRootAsRoot(buf, 0)

    for i in range(root.StudentsLength()):
        student = root.Students(i)

        name = btos(student.Name())
        login = btos(student.Login())
        group = btos(student.Group())
        practice = [student.Practice(j) for j in range(student.PracticeLength())]
        repo = btos(student.Project().Repo())
        project_mark = student.Project().Mark()
        mark = student.Mark()

        student_data = struct.pack(
            student_format,
            bytes(name, 'utf-8'),
            bytes(login, 'utf-8'),
            bytes(group, 'utf-8'),
            *practice,
            bytes(repo, 'utf-8'),
            project_mark,
            mark
        )
        output_file.write(student_data)
        # print("Successfully transformed!")


def bin_to_flat(input_file: FileIO, output_file: FileIO):
    students = []
    builder = flatbuffers.Builder(2048)
    while True:
        student_data = input_file.read(struct.calcsize(student_format))
        if not student_data:
            break

        name, login, group, *practice, repo, project_mark, mark = struct.unpack(student_format, student_data)

        Name = builder.CreateString(btos(name))
        Login = builder.CreateString(btos(login))
        Group = builder.CreateString(btos(group))

        Student.StudentStartPracticeVector(builder, 8)

        for i in range(len(practice)):
            builder.PrependByte(practice[len(practice) - 1 - i])

        prac = builder.EndVector(8)
        repo = builder.CreateString(btos(repo))

        Project.ProjectStart(builder)
        Project.ProjectAddRepo(builder, repo)
        Project.ProjectAddMark(builder, project_mark)
        project_obj = Project.ProjectEnd(builder)

        Student.StudentStart(builder)
        Student.StudentAddName(builder, Name)
        Student.StudentAddLogin(builder, Login)
        Student.StudentAddGroup(builder, Group)
        Student.StudentAddPractice(builder, prac)
        Student.StudentAddProject(builder, project_obj)
        Student.StudentAddMark(builder, mark)

        students.append(Student.StudentEnd(builder))

    Root.RootStartStudentsVector(builder, len(students))
    for i in range(len(students)):
        builder.PrependUOffsetTRelative(students[len(students) - 1 - i])

    students_vec = builder.EndVector(len(students))

    Root.RootStart(builder)
    Root.RootAddStudents(builder, students_vec)
    root = Root.RootEnd(builder)

    builder.Finish(root)
    buf = builder.Output()
    output_file.write(buf)
    # print("Successfully transformed!")


def btos(s):
    return s.decode('utf-8').rstrip('\0')


def output(path):
    file = open(path, 'wb')
    return file


def main():
    input_file = open(sys.argv[1], 'rb')
    input_file_extension = os.path.splitext(sys.argv[1])[1]
    if input_file_extension == '.bin':
        output_path = os.path.splitext(sys.argv[1])[0] + '.flat'
        output_file = output(output_path)
        bin_to_flat(input_file, output_file)
    elif input_file_extension == '.flat':
        output_path = os.path.splitext(sys.argv[1])[0] + '.bin'
        output_file = output(output_path)
        flat_to_bin(input_file, output_file)
    else:
        print('Invalid file extension')
    input_file.close()

if __name__ == "__main__":
  main()