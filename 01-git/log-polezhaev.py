#!/usr/bin/python3

import argparse
import os

COMMIT_FILENAME = ".commit:\t"
PARENT_ROOT_FILENAME = ".parent/\t"
COMMIT_START_PATTERN = "Root:"

def ParseArgs():
    parser = argparse.ArgumentParser(
        prog='Log',
        description='Program imitates `git log` command'
    )
    parser.add_argument('root_hash')
    args = parser.parse_args()

    root_hash = args.root_hash
    return root_hash

def GetPathsByName(content, filename):
    correct_files = []
    for file in content:
        if file.startswith(filename):
            correct_files.append(file[len(filename):-1])
    return correct_files

def PrintCommitsContent(commits):
    for commit in commits:
        if commit not in os.listdir('.'):
            print(f"Non-existing commit hash: {commit}")
            os._exit(1)
        with open(commit, 'r') as file:
            first_line = file.readline()
            if first_line.startswith(COMMIT_START_PATTERN):
                print(first_line, file.read(), sep="")
                return first_line[len(COMMIT_START_PATTERN) + 1:-1]
    return ""

if __name__ == "__main__":
    now_root = ParseArgs()
    while now_root != "":
        if now_root not in os.listdir('.'):
            print(f"Non-existing root hash: {now_root}")
            os._exit(1)
        with open(now_root, 'r') as file:
            root_content = file.readlines()

        commits = GetPathsByName(root_content, COMMIT_FILENAME)
        now_root = PrintCommitsContent(commits)

        