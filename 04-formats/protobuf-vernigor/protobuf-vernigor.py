import os
import sys
import struct as Struct
import student_pb2 as Student

NAME_LAST_POS = 32
LOGIN_LAST_POS = 48
GROUP_LAST_POS = 56
PRACTICE_LAST_POS = 64
PROJECT_REPO_LAST_POS = 123
PROJECT_MARK_LAST_POS = 124
MARK_LAST_POS = 125

FORMAT_STRING = "<32c16c8c8B59cBf"

PROTOBUF_EXTENSION = "protobuf"
BIN_EXTENSION = "bin"

PROTOBUF_MULTIPLIER = 142
BIN_MULTIPLIER = 128

def get_output_file_path(input_file_path, extension):
    return input_file_path.rsplit('.', 1)[0] + "." + extension

def clear_output_file_if_exist(path):
    open(path, 'w').close()

def read_file(file_name):
    # Reading data from bin file
    in_file = open(file_name, "rb")
    data = in_file.read() 
    in_file.close()

    return data

def dump_file(data, path):
    with open(path, "ab") as f:
        # binary output
        f.write(data)

def unpack_data(data):
    # Unpack data from binary format
    unpacked_tuple = list(Struct.unpack(FORMAT_STRING, data))
    
    name = unpacked_tuple[0:NAME_LAST_POS]

    login = unpacked_tuple[NAME_LAST_POS:LOGIN_LAST_POS]
    group = unpacked_tuple[LOGIN_LAST_POS:GROUP_LAST_POS]

    practice = unpacked_tuple[GROUP_LAST_POS:PRACTICE_LAST_POS]

    project_repo = unpacked_tuple[PRACTICE_LAST_POS:PROJECT_REPO_LAST_POS]
    mark_repo = unpacked_tuple[PROJECT_REPO_LAST_POS:PROJECT_MARK_LAST_POS]

    mark = unpacked_tuple[PROJECT_MARK_LAST_POS:MARK_LAST_POS]

    return [name, login, group, practice, project_repo, mark_repo, mark]

def process_unpacked_data(unpacked_data):
    name = (b''.join(unpacked_data[0])).decode()
    login =(b''.join(unpacked_data[1])).decode()
    group = (b''.join(unpacked_data[2])).decode()

    practice = unpacked_data[3]

    project_repo = (b''.join(unpacked_data[4])).decode()
    project_mark = unpacked_data[5][0]

    mark = unpacked_data[6][0]

    return [name, login, group, practice, project_repo, project_mark, mark]

def build_student_proto(processed_data):
    # Fill in student structure
    student = Student.Student()
    student.name = processed_data[0]
    student.login = processed_data[1]
    student.group = processed_data[2]
    student.practice.extend(processed_data[3])
    student.project.repo = processed_data[4]
    student.project.mark = processed_data[5]
    student.mark = processed_data[6]
    return student

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
    print(f"{os.path.getsize(path) // BIN_MULTIPLIER} students read...")
    print("written to  " + output_file_path)

def pack_data(processed_data):
    return Struct.pack(FORMAT_STRING,
     *processed_data[0], *processed_data[1],
     *processed_data[2], *processed_data[3],
     *processed_data[4], processed_data[5], processed_data[6])

def from_student_to_cformat_data(student):
    def get_list_of_bytes_from_string(string):
        return list(map(lambda x: x.to_bytes(1, 'little'), string.encode()))

    name = get_list_of_bytes_from_string(student.name)
    login = get_list_of_bytes_from_string(student.login)
    group = get_list_of_bytes_from_string(student.group)

    practice = student.practice

    project_repo = get_list_of_bytes_from_string(student.project.repo)
    project_mark = student.project.mark
    
    mark = student.mark

    return [name, login, group, practice, project_repo, project_mark, mark]

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


def get_file_type(file_name):
    return file_name.split('.')[-1]

def check_file_size(path, multiplier):
    if os.path.getsize(path) % multiplier == 0:
        return True
    print(f"File size should be a multiplier of {multiplier}")
    return False

            

path = sys.argv[1]
file_type = get_file_type(path)
if file_type == PROTOBUF_EXTENSION:
    if check_file_size(path, PROTOBUF_MULTIPLIER):
        protobuf_to_bin(sys.argv[1])
elif file_type == BIN_EXTENSION:
    if check_file_size(path, BIN_MULTIPLIER):
        bin_to_protobuf(sys.argv[1])
else:
    print("Only bin and protobuf extensions are accepted.")
