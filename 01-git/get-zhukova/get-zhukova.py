#!/usr/bin/python3

import argparse
import os
from sha256sum import sha256sum

DIR_SEPARATOR = '/\t'
FILE_SEPARATOR = ':\t'

def parse_arguments():
    parser = argparse.ArgumentParser(
                        prog='Git Get',
                        description='Program that imitates `get` command')
    parser.add_argument('path')
    parser.add_argument('hash')
    args = parser.parse_args()

    root_hash, path = args.hash, args.path.split('/')
    if '' in path:
        print(f"Invalid path {path}, empty tokens found")
        os._exit(1)
    return root_hash, path


def find_new_root_hash_for_blob_in_root_hash(root_hash_data, blob_name, separator):
    lines = root_hash_data.split('\n')
    for line in lines:
        tokens = line.split(separator)
        print(tokens, separator in line)
        if len(tokens) != 2:
            continue
        name, root_hash = tokens
        if name.strip() == blob_name:
            return root_hash.strip()
    return None


if __name__ == "__main__":
    root_hash, path = parse_arguments()
    prefix = '.'

    for i in range(len(path)):
        token = path[i]
        if root_hash not in os.listdir('.'):
            print(f"Non-existing root hash: {root_hash}")
            os._exit(1)
        with open(root_hash, 'r') as f:
            data = f.read()
        prefix = f"{prefix}/{token}"
        if root_hash != sha256sum(root_hash):
            print(f"Integrity of {prefix} is corrupted: {root_hash} vs {sha256sum(root_hash)}")
            os._exit(1)
        if i == len(path) - 1:
            root_hash = find_new_root_hash_for_blob_in_root_hash(data, token, FILE_SEPARATOR)
            if root_hash is None:
                print(f"Failed to find {token}")
                os._exit(1)
            with open(root_hash, 'r') as f:
                print(f.read())
            break
        root_hash = find_new_root_hash_for_blob_in_root_hash(data, token, DIR_SEPARATOR)
        if root_hash is None:
            print(f"Failed to find {token}")
            os._exit(1)