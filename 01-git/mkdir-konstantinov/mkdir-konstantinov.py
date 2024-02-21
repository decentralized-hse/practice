import argparse
import hashlib
import os

def parse_arguments():
    parser = argparse.ArgumentParser(
                        prog='Git mkdir',
                        description='Program that imitates `mkdir` command')
    parser.add_argument('path')
    parser.add_argument('hash')
    args = parser.parse_args()

    root_hash, path = args.hash, args.path
    return root_hash, path

def mkdir(directory, prev_root_hash):
    prev_file_list = get_file_list(prev_root_hash)

    prev_file_list.remove('')
    parent = [s for s in prev_file_list if "./parent" in s]
    if len(parent) > 0:
        prev_file_list.remove([s for s in prev_file_list if "./parent" in s][0])
    new_dir_hash = create_blob(directory)
    new_file_list = prev_file_list + [directory + '/ ' + new_dir_hash] + ['.parent/ ' + prev_root_hash]
    new_file_list.sort()

    new_directory_str = '\n'.join(new_file_list)
    new_root_hash = hashlib.sha256(new_directory_str.encode('utf-8')).hexdigest()
    new_directory_path = new_root_hash
    if not os.path.exists(new_directory_path):
        with open(new_directory_path, 'w') as file:
            file.write(new_directory_str)

    return new_root_hash


def get_file_list(root_hash):
    directory_path = root_hash
    with open(directory_path, 'r') as file:
        file_list = file.read().split('\n')
    return file_list


def create_blob(data):
    blob_hash = hashlib.sha256(data.encode('utf-8')).hexdigest()
    blob_path = blob_hash
    if not os.path.exists(blob_path):
        with open(blob_path, 'w') as file:
            file.write('')
    return blob_hash


if __name__ == "__main__":
    root_hash, path = parse_arguments()
    new_root_hash = mkdir(path, root_hash)
    print("New root hash:", new_root_hash)