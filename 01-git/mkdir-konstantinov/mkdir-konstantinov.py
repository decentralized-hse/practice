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
    parent = [s for s in prev_file_list if ".parent/" in s]
    if len(parent) > 0:
        prev_file_list.remove(parent[0])
    new_dir_hash = create_blob('')
    new_file_list = prev_file_list + ['.parent/\t' + prev_root_hash]

    if '/' in directory:
        dir_list = directory.split('/')
        parent_dir = prev_root_hash
        for i in range(len(dir_list)):
            if i == len(dir_list) - 1:
                with open(parent_dir,'a') as prev_dir:
                    prev_dir.write(dir_list[i] + '/\t' + new_dir_hash + '\n')
            else:
                with open(parent_dir,'r') as dir:
                    dir_read = dir.read()
                    if dir_list[i] in dir_read:
                        prev_dir_list = get_file_list(parent_dir)
                        parent_dir = [s for s in prev_dir_list if dir_list[i] in s][0].split('\t')[1]
                    else:
                        print(f"{dir_list[i]}: No such file or directory")
                        os._exit(1)
    else:
        new_file_list += [directory + '/\t' + new_dir_hash]
    new_file_list.sort()
    new_directory_str = '\n'.join(new_file_list) + '\n'
    new_root_hash = hashlib.sha256(new_directory_str.encode('utf-8')).hexdigest()
    new_directory_path = new_root_hash
    if not os.path.exists(new_directory_path):
        with open(new_directory_path, 'w') as file:
            file.write(new_directory_str)

    return new_root_hash


def get_file_list(root_hash):
    directory_path = root_hash
    with open(directory_path, 'r') as file:
        file_read = file.read()
        file_list = file_read.split('\n')
    print(file_list[0].split('\t'))
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
