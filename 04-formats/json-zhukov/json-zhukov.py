import sys
import json
from ctypes import *


class Project(Structure):
    _fields_ = (
        ("repo", c_char * 59),
        ("mark", c_uint8),
    )

    def to_dict(self):
        return {
            "repo": self.repo.decode(),
            "mark": self.mark,
        }


class Student(Structure):
    _fields_ = (
        ("name", c_char * 32),
        ("login", c_char * 16),
        ("group", c_char * 8),
        ("practice", c_uint8 * 8),
        ("project", Project),
        ("mark", c_float),
    )

    def to_dict(self):
        return {
            "name": self.name.decode(),
            "login": self.login.decode(),
            "group": self.group.decode(),
            "practice": list(self.practice),
            "project": self.project.to_dict(),
            "mark": self.mark,
        }


def bin_to_json(inp_file, out_file):
    with open(inp_file, "rb") as file:
        students_dict = []
        student = Student()
        while file.readinto(student) == sizeof(student):
            students_dict.append(student.to_dict())

    with open(out_file, "w") as file:
        json.dump(students_dict, file, ensure_ascii=False, indent=2)


def json_to_bin(inp_file, out_file):
    with open(inp_file, "r") as file:
        students_dict = json.load(file)

    students = []
    for student_dict in students_dict:
        project_dict = student_dict["project"]
        project_dict["repo"] = project_dict["repo"].encode()
        student_dict["project"] = Project(*project_dict.values())

        student_dict["practice"] = (c_uint8 * 8)(*student_dict["practice"])
        student_dict = {k: (v.encode() if isinstance(v, str) else v) for k, v in student_dict.items()}
        students.append(Student(*student_dict.values()))

    with open(out_file, "wb") as file:
        file.writelines(students)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise TypeError("Missing path to file parameter")
    inp_file = sys.argv[1]

    if inp_file.endswith(".bin"):
        bin_to_json(inp_file, f"{inp_file[:-4]}.json")
    elif inp_file.endswith(".json"):
        json_to_bin(inp_file, f"{inp_file[:-5]}.bin")
    else:
        raise ValueError("Incorrect file extension: should be bin or json")
