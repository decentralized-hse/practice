#!/usr/bin/python3

import argparse
import os
import sys
import time

from datetime import datetime
from tempfile import NamedTemporaryFile
from sha256sum import sha256sum


def parse_arguments():
    parser = argparse.ArgumentParser(
        prog="git commit",
        description="Program that imitates `commit` command",
    )
    parser.add_argument("roothash")
    args = parser.parse_args()
    return args


def get_commit_full_text(roothash: str):
    now = datetime.now().strftime("%d %b %Y %H:%M:%S")
    result = [
        f"Root: {roothash}",
        f"Date: {now} {time.tzname[0]}",
        "",
        sys.stdin.read(),
    ]
    return result


def get_new_root_text(commit_hash: str, roothash: str):
    new_root_lines = []
    with open(roothash, "r") as f:
        for line in f.readlines():
            if line.startswith(".commit"):
                continue
            if line.startswith(".parent"):
                continue
            new_root_lines.append(line.rstrip())
    new_root_lines.append(f".commit:\t{commit_hash}")
    new_root_lines.append(f".parent/\t{roothash}")
    new_root_lines = sorted(new_root_lines)
    return new_root_lines

def save_lines_to_file(lines: list[str]):
    tmp = ".ershov-tmp"
    with open(tmp, "w") as f:
        for line in lines:
            print(line, file=f)
    hash_ = sha256sum(tmp)
    os.rename(tmp, hash_)
    return hash_


if __name__ == "__main__":
    args = parse_arguments()
    files = [f for f in os.listdir(".") if os.path.isfile(f)]
    assert args.roothash in files, f"Incorrect Root hash {args.roothash}"

    commit_text = get_commit_full_text(args.roothash)
    commit_hash = save_lines_to_file(commit_text)
    new_root_text = get_new_root_text(commit_hash, args.roothash)
    new_rool_hash = save_lines_to_file(new_root_text)
    print(new_rool_hash)
