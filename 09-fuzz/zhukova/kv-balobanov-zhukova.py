from __future__ import annotations

import argparse
import ast
import struct

from dataclasses import dataclass, asdict
from typing import Final

STRUCT_SIZE: Final[int] = 128
# BUG-FIX: некоторые символы не парсились из-за стандарта utf-8.
STANDART = "ISO-8859-1"
ZERO_BYTE = b"\x00"

@dataclass
class Student:
    _id: int
    name: str
    login: str
    group: str
    practice: list[int]
    project_repo: str
    project_mark: int
    mark: float

    def get_tuple(self) -> tuple:
        attrs = []
        for attr_name, attr_val in asdict(self).items():
            if attr_name == "_id":
                continue
            if isinstance(attr_val, str):
                attrs.append(attr_val.encode(STANDART))
            elif isinstance(attr_val, list):
                attrs.extend(attr_val)
            else:
                attrs.append(attr_val)
        return tuple(attrs)

    def serialize_b(self) -> bytes:
        result = struct.pack("<32s16s8s8B59sBf", *self.get_tuple())
        while ZERO_BYTE in result:
            result = result.replace(ZERO_BYTE, b"")
        return result

    def serialize_kv(self) -> str:
        lines = []
        for attr_name, attr_val in asdict(self).items():
            if attr_name == "_id":
                continue
            if attr_name == "practice":
                # BUG-FIX: не обрабатывался массив заполненный не полностью.
                practice_marks = [f"[{self._id}].{attr_name}.[{i}]: {attr_val[i]}" for i in range(len(attr_val))]
                practice_marks = practice_marks + [f"[{self._id}].{attr_name}.[{i}]: {0}" for i in range(len(practice_marks), 8)]
                lines.extend(practice_marks)
            else:
                lines.append("[{}].{}: {}".format(self._id, attr_name.replace('_', '.'), repr(attr_val)))

        return "\n".join(lines) + "\n"

    @staticmethod
    def parse_practice(lines: list[str]) -> list[int]:
        marks = []
        for line in lines:
            marks.append(int(line.split(": ")[1]))

        return marks

    @staticmethod
    def parse_line(line: str) -> tuple:
        key, value = line.split(": ")
        name = []
        for item in key.split("."):
            if item.startswith("["):
                continue
            name.append(item)

        return "_".join(name), value

    @classmethod
    def deserialize_kv(cls, student_struct: list[str]) -> Student:
        print(student_struct)
        i = 0
        d = {}
        while i < len(student_struct):
            if "practice" in student_struct[i].split(":")[0]:
                # BUG-FIX: не обрабатывался массив заполненный не полностью.
                ind = 8
                for j in range(i, i + 8):
                    if "practice" not in student_struct[j].split(":")[0]:
                        ind = j
                        break
                d["practice"] = cls.parse_practice(student_struct[i:ind])
                d["practice"] = d["practice"] + [0] * (8 - len(d["practice"]))
                i = ind
            else:
                key, val = cls.parse_line(student_struct[i])
                if key == "mark":
                    d[key] = float(val)
                elif key == "project_mark":
                    d[key] = int(val)
                else:
                    d[key] = ast.literal_eval(val)
                i += 1

        return Student(_id=0, **d)

    @classmethod
    def deserialize_b(cls, student_struct: bytes, id: int) -> Student:
        # BUG-FIX: было ограничение на то, что в файл записано кратное STRUCT_SIZE число символов.
        student_struct = student_struct + ZERO_BYTE * (STRUCT_SIZE - len(student_struct))
        
        return cls(
            _id=id,
            name=student_struct[:32].decode(STANDART).rstrip("\0"),
            login=student_struct[32:48].decode(STANDART).rstrip("\0"),
            group=student_struct[48:56].decode(STANDART).rstrip("\0"),
            practice=list(student_struct[56:64]),
            project_repo=student_struct[64:123].decode(STANDART).rstrip("\0"),
            project_mark=struct.unpack("<B", student_struct[123:124])[0],
            mark=struct.unpack("<f", student_struct[124:128])[0])

def proceed_file(filename):
    if filename.endswith(".bin"):
        with open(filename, "rb") as in_file, open(f"{filename[:-4]}.kv", "w") as out_file:
            counter = 0
            data = in_file.read(STRUCT_SIZE)
            while data:
                out_file.write(Student.deserialize_b(data, counter).serialize_kv())
                counter += 1
                data = in_file.read(STRUCT_SIZE)
            print(f"{counter} students read...")
            print(f"written to {filename[:-4]}.kv...")
    elif filename.endswith(".kv"):
        with open(filename, "r") as in_file, open(f"{filename[:-3]}.bin", "wb") as out_file:
            lines = []
            counter = 0
            for line in in_file:
                lines.append(line[:-1])
                if len(lines) == 14:
                    out_file.write(Student.deserialize_kv(lines).serialize_b())
                    lines.clear()
                    counter += 1
            if len(lines) > 0:
                out_file.write(Student.deserialize_kv(lines).serialize_b())
                counter += 1
            print(f"{counter} students read...")
            print(f"written to {filename[:-3]}.bin...")
    else:
        raise Exception("Invalid file format")


import os
import sys
import frelatage

LOG_FILE = "out.log"
FILE_NAME = "test"
PROG = "kv-balobanov-zhukova.py"

def prapare():
    os.system(f"gcc ../bin-gritzko-check.c -o ./checker.o")

def validate_input(file_name: str):
    code = os.system(f"./checker.o {file_name} 2> {LOG_FILE}")
    exit_status = os.WEXITSTATUS(code)
    return exit_status == 0

def encode_data(file_name: str):
    print('! encoding: .kv -> .bin', file=sys.stderr)
    code = os.system(f"python3 {PROG} serializer -f {file_name} > {LOG_FILE}")
    exit_status = os.WEXITSTATUS(code)
    if exit_status != 0:
        os._exit(1)

def decode_data(file_name: str):
    print('! decoding: .bin -> .kv', file=sys.stderr)
    code = os.system(f"python3 {PROG} serializer -f {file_name} > {LOG_FILE}")
    exit_status = os.WEXITSTATUS(code)
    if exit_status != 0:
        os._exit(1)

def kv_parser_round_trip_executor(data):
    bin_fname = f"{FILE_NAME}.bin"
    kv_fname = f"{FILE_NAME}.kv"
    with open(bin_fname, "w") as f:
        f.write(data)
    if not validate_input(bin_fname):
        return
    try:
        decode_data(bin_fname)
        encode_data(kv_fname)
    except:
        os._exit(1)
    with open(bin_fname, "r") as f:
        if f.read() != data:
            with open("sample", "w") as f:
                f.write(data + "\nvs\n" + f.read())
            os._exit(1)


def run_fuzzer():
    prapare()
    input = frelatage.Input(value="initial_value")
    f = frelatage.Fuzzer(kv_parser_round_trip_executor, [[input]])
    f.fuzz()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                        prog='Fuzzer & KV serializer deserializer',
                        description='Program that imitates `get` command')
    parser.add_argument('mode', default="fuzzer", choices=["fuzzer", "serializer"])
    parser.add_argument("-f", '--file', type=str)

    args = parser.parse_args()
    mode = args.mode
    if mode == "fuzzer":
        run_fuzzer()
    elif mode == "serializer":
        filename = args.file
        proceed_file(filename)