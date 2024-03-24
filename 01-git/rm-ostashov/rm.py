import argparse
import re
import hashlib
import os
import sys
from typing import List

class EntryType:
    Blob = 'Blob'
    Tree = 'Tree'


class Entry:
    def __init__(self, name, hash, type):
        self.name = name
        self.hash = hash
        self.type = type


class Tree:
    blob_matcher = re.compile(r"^([^\t :/]+):\t([a-fA-F0-9]{64})$")
    tree_matcher = re.compile(r"^([^\t :/]+)/\t([a-fA-F0-9]{64})$")

    @staticmethod
    def parse_line(line):
        if Tree.blob_matcher.match(line):
            entry_type = EntryType.Blob
        elif Tree.tree_matcher.match(line):
            entry_type = EntryType.Tree
        else:
            raise ValueError("Incorrect format of tree file")

        pieces_match = Tree.blob_matcher.match(line) or Tree.tree_matcher.match(line)
        name = pieces_match.group(1)
        hash = pieces_match.group(2)
        return Entry(name, hash, entry_type)

    def __init__(self, entries):
        self.entries = entries

    def find_it(self, name):
        for entry in self.entries:
            if entry.name == name:
                return entry
        raise ValueError(f"Entry: '{name}' not found in tree")

    def serialize(self):
        lines = []
        for entry in self.entries:
            if entry.type == EntryType.Blob:
                lines.append(f"{entry.name}:\t{entry.hash}\n")
            else:
                lines.append(f"{entry.name}/\t{entry.hash}\n")
        return "".join(lines)

    @staticmethod
    def from_hash(hash):
        if not os.path.exists(hash):
            raise ValueError(f"Tree: '{hash}' not found")

        with open(hash, 'r') as tree_file:
            entries = [Tree.parse_line(line) for line in tree_file]

        return Tree(entries)

    def update_parent(self, hash):
        for entry in self.entries:
            if entry.name == ".parent":
                entry.hash = hash
                return
        self.entries.append(Entry(".parent", hash, EntryType.Tree))

    def find_entry(self, name):
        return self.find_it(name)

    def remove_entry(self, name):
        self.entries = [entry for entry in self.entries if entry.name != name]

    def replace_hash(self, name, hash):
        self.find_it(name).hash = hash

    def write_to_file(self):
        data = self.serialize()
        hash = hashlib.sha256(data.encode('utf-8')).hexdigest()

        with open(hash, 'w') as tree_file:
            tree_file.write(data)

    def calculate_hash(self):
        return hashlib.sha256(self.serialize().encode('utf-8')).hexdigest()


def check_args(path, root_hash):
    path_matcher = re.compile("^[^\t :]+$")
    hash_matcher = re.compile("^[a-fA-F0-9]{64}$")

    if not path_matcher.match(path):
        raise ValueError("Incorrect format of path")

    if not hash_matcher.match(root_hash):
        raise ValueError("Incorrect format of root hash")


def split_path(path) -> List[str]:
    return path.split("/")


def run_remove(path_parts, root_hash):
    trees = [Tree.from_hash(root_hash)]

    for i in range(len(path_parts) - 1):
        entry = trees[-1].find_entry(path_parts[i])
        if entry.type != EntryType.Tree:
            raise RuntimeError(f"'{path_parts[i]}' is not a folder")
        trees.append(Tree.from_hash(entry.hash))

    trees.reverse()
    path_parts.reverse()

    trees[-1].update_parent(root_hash)
    trees[0].remove_entry(path_parts[0])
    trees[0].write_to_file()

    for i in range(1, len(trees)):
        trees[i].replace_hash(path_parts[i], trees[i - 1].calculate_hash())
        trees[i].write_to_file()

    return trees[-1].calculate_hash()


def main():
    parser = argparse.ArgumentParser(description='Remove the specified path from the tree')
    parser.add_argument('path', help='path to remove')
    parser.add_argument('root_hash', help='hash of root tree')
    args = parser.parse_args()

    path = args.path
    root_hash = args.root_hash
    check_args(path, root_hash)

    path_parts = split_path(path)
    new_root_hash = run_remove(path_parts, root_hash)

    print(new_root_hash)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e, file=sys.stderr)
        sys.exit(1)
