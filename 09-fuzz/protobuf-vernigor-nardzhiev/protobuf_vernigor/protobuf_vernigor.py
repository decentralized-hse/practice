import os
import sys
import student_pb2 as Student
from google.protobuf.internal.decoder import _DecodeVarint32


from consts import *
from utils import *
from converters import *


def bin_to_protobuf(path):
    print("Reading binary student data from " + path  + "...")

    output_file_path = get_output_file_path(path, PROTOBUF_EXTENSION)
    clear_output_file_if_exist(output_file_path)

    data = read_file(path)
    for i in range(os.path.getsize(path) // BIN_MULTIPLIER):
        unpacked_data = unpack_data(data[i * BIN_MULTIPLIER : (i + 1) * BIN_MULTIPLIER])
        processed_data = process_unpacked_data(unpacked_data)

        student = build_student_proto(processed_data)
        dump_pb_file(student, output_file_path)
    print(f"{os.path.getsize(path) // BIN_MULTIPLIER} students read...")
    print("written to  " + output_file_path)


def protobuf_to_bin(path):
    print("Reading binary student data from " + path  + "...")
    
    data = read_file(path)

    output_file_path = get_output_file_path(path, BIN_EXTENSION)
    clear_output_file_if_exist(output_file_path)

    students_number = 0
    pos = 0
    while pos < len(data):
        try:
            msg_len, pos = _DecodeVarint32(data, pos)
            msg_buf = data[pos: pos + msg_len]
            pos += msg_len
            student = Student.Student()
            student.ParseFromString(msg_buf)
        except Exception as e:
            raise ValueError("Malformed input")
        
        students_number += 1
        processed_data = from_student_to_cformat_data(student)
        packed_data = pack_data(processed_data)
        dump_file(packed_data, output_file_path)
        
    print(f"{students_number} students read...")
    print("written to  " + output_file_path)


if __name__ == "__main__":
    path = sys.argv[1]
    file_type = get_file_type(path)
    if file_type == PROTOBUF_EXTENSION:
        try:
            protobuf_to_bin(sys.argv[1])
        except Exception:
            sys.stderr.write("Malformed input")
            sys.exit(-1)
    elif file_type == BIN_EXTENSION:
        try:
            bin_to_protobuf(sys.argv[1])
        except Exception:
            sys.stderr.write("Malformed input")
            sys.exit(-1)
    else:
        sys.stderr.write("Malformed input")
        sys.exit(-1)

