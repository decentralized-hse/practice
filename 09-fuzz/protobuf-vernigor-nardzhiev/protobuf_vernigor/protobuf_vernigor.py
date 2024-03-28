import os
import sys
import student_pb2 as Student

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
        dump_file(student.SerializeToString(), output_file_path)
    if not check_file_size(output_file_path, PROTOBUF_MULTIPLIER):
        raise ValueError("Malformed input")
    print(f"{os.path.getsize(path) // BIN_MULTIPLIER} students read...")
    print("written to  " + output_file_path)


def protobuf_to_bin(path):
    print("Reading binary student data from " + path  + "...")
    
    data = read_file(path)

    output_file_path = get_output_file_path(path, BIN_EXTENSION)
    clear_output_file_if_exist(output_file_path)

    for i in range(os.path.getsize(path) // PROTOBUF_MULTIPLIER):
        student = Student.Student()
        student.ParseFromString(data[PROTOBUF_MULTIPLIER * i: PROTOBUF_MULTIPLIER * (i + 1)])

        processed_data = from_student_to_cformat_data(student)
        packed_data = pack_data(processed_data)

        dump_file(packed_data, output_file_path)
    
    print(f"{os.path.getsize(path) // PROTOBUF_MULTIPLIER} students read...")
    print("written to  " + output_file_path)


if __name__ == "__main__":
    path = sys.argv[1]
    file_type = get_file_type(path)
    if file_type == PROTOBUF_EXTENSION:
        if not check_file_size(path, PROTOBUF_MULTIPLIER):
            sys.stderr.write("Malformed input")
            sys.exit(-1)
        protobuf_to_bin(sys.argv[1])
    elif file_type == BIN_EXTENSION:
        if not check_file_size(path, BIN_MULTIPLIER):
            sys.stderr.write("Malformed input")
            sys.exit(-1)
        bin_to_protobuf(sys.argv[1])
    else:
        sys.stderr.write("Malformed input")
        sys.exit(-1)

