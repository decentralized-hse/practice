import os
import struct as Struct

from consts import *

def get_file_type(file_name):
    return file_name.split('.')[-1]

def check_file_size(path, multiplier):
    if os.path.getsize(path) % multiplier == 0:
        return True
    print(f"File size should be a multiplier of {multiplier}")
    return False

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

def pack_data(processed_data):
    return Struct.pack(FORMAT_STRING,
     *processed_data[0], *processed_data[1],
     *processed_data[2], *processed_data[3],
     *processed_data[4], processed_data[5], processed_data[6])

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
    try:
        name = (b''.join(unpacked_data[0])).decode()
        login =(b''.join(unpacked_data[1])).decode()
        group = (b''.join(unpacked_data[2])).decode()

        practice = unpacked_data[3]

        project_repo = (b''.join(unpacked_data[4])).decode()
        project_mark = unpacked_data[5][0]

        mark = unpacked_data[6][0]
    except UnicodeDecodeError as e:
        raise ValueError("Malformed input")

    return [name, login, group, practice, project_repo, project_mark, mark]
