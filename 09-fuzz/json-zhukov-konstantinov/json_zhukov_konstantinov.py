import argparse
import os
import sys
import json
from ctypes import *
import frelatage

FILE_NAME = "json-zhukov-konstantinov.py"

class Project(Structure):
    _fields_ = (
        ("repo", c_char * 59),
        ("mark", c_uint8),
    )

    def to_dict(self):
        return {
            "repo": self.repo.decode("utf-8").rstrip('\0'),
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
            "name": self.name.decode("utf-8").rstrip('\0'),
            "login": self.login.decode("utf-8").rstrip('\0'),
            "group": self.group.decode("utf-8").rstrip('\0'),
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
        project_dict["repo"] = project_dict["repo"].encode("utf-8")
        student_dict["project"] = Project(*project_dict.values())

        student_dict["practice"] = (c_uint8 * 8)(*student_dict["practice"])
        student_dict = {k: (v.encode("utf-8") if isinstance(v, str) else v) for k, v in student_dict.items()}
        students.append(Student(*student_dict.values()))

    with open(out_file, "wb") as file:
        file.writelines(students)


def serialize(inp_file):
    if inp_file.endswith(".bin"):
        bin_to_json(inp_file, f"{inp_file[:-4]}.json")
    elif inp_file.endswith(".json"):
        json_to_bin(inp_file, f"{inp_file[:-5]}.bin")
    else:
        raise ValueError("Incorrect file extension: should be bin or json")


def frelatage_fuzzer(data):
    bin_file = 'test.bin'
    json_file = 'test.json'
    with open(bin_file, "w") as f:
        f.write(data)
    code = os.system(f"./checker.o {bin_file} 2> out.log")
    exit_status = os.WEXITSTATUS(code)
    if exit_status != 0:
        return
    try:
        serialize(bin_file)
        serialize(json_file)
    except Exception as e:
        print("FIND EXCEPTION:",e)
        os._exit(1)
    with open(bin_file, "r") as f:
        if f.read() != data:
            with open("sample", "w") as f:
                f.write(data + "\nvs\n" + f.read())
                print("FIND ERROR!!!!!!!")
            os._exit(1)

def run_fuzz():
    try:
        os.system(f"gcc ../bin-gritzko-check.c -o ./checker.o")
        input = frelatage.Input(value="initial_value")
        f = frelatage.Fuzzer(frelatage_fuzzer, [[input]])
        f.fuzz() 
    except Exception as e:
        print("FIND EXCEPTION:",e)
        os._exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                        prog='Fuzzer',
                        description='Fuzzer')
    parser.add_argument('mode', default="fuzzer", choices=["fuzzer", "serializer"])
    parser.add_argument("-f", '--file', type=str)
    args = parser.parse_args()
    mode = args.mode
    if mode == "fuzzer":
        run_fuzz()
    elif mode == "serializer":
        filename = args.file
        serialize(filename)