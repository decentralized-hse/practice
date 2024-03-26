#!/usr/bin/python3

import sys
import os

def ls(root, prefix):
    with open(root, 'r') as file:
        for line in file:
            splitted = line.strip().split("\t", 1)

            if len(splitted) < 2:
                raise Exception("FS format compromised, probably not a directory")

            name, *hash = splitted[0], splitted[1]
            hash = next(iter(hash), None)

            if name.endswith(':'):
                print(prefix + name[:-1])
            elif name.endswith('/'):
                if name[:-1] == ".parent":
                    continue
                print(prefix + name)
                ls(hash, prefix + "/" + name)
            else:
                raise Exception("FS format compromised")


def find(path, root):
    dir = "."
    while dir == "." and path is not None:
        dir, *path = path.split("/", 1)
        path = next(iter(path), None)

    if path is None:
        return root

    dir += "/"

    with open(root, 'r') as file:
        for line in file:
            name, *hash = line.strip().split("\t", 1)
            hash = next(iter(hash), None)

            if dir == name:
                return find(path, hash.strip())
    raise Exception(f"Directory not found")

def main():
    argv = sys.argv
    if len(argv) != 3:
        raise Exception("Wrong arguments! usage: ./ls-belyakov <path> <root-hash>")
    path = argv[1]
    root = argv[2]
    hash = find(path, root)
    ls(hash, "")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(str(e))
        exit(1)
