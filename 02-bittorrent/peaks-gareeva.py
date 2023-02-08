import os
import sys
from typing import List, Optional

LEVELS: int = 32
HASH_SIZE: int = 64
NULL: str = "0" * HASH_SIZE + "\n"


# https://research.swtch.com/tlog#appendix_b
def get_index(level: int, offset: int) -> int:
    if level == 0:
        return offset << 1
    return (1 << (level + 1)) * offset + (1 << level) - 1


def get_node(hashtree: List[str], index: int) -> Optional[str]:
    if index < len(hashtree):
        node = hashtree[index]
        return node if node != NULL else None
    return None


def reading_hashtree(filename: str) -> List[str]:
    filename = f"{filename}.hashtree"
    print(f"reading {filename}...")
    if not os.path.isfile(filename):
        raise Exception(f"File with name {filename} does not exist.")
    with open(filename) as f:
        return f.readlines()


def get_peaks(hashtree: List[str]) -> List[str]:
    peaks: List[str] = [NULL] * LEVELS
    checked_tree_offset = 0
    for level in range(LEVELS - 1, -1, -1):
        index = get_index(level, checked_tree_offset)
        node = get_node(hashtree, index)
        if node:
            peaks[level] = node
            checked_tree_offset = checked_tree_offset + 1
        checked_tree_offset = checked_tree_offset << 1
    return peaks


def putting_peaks(filename: str, peaks: List[str]):
    filename = f"{filename}.peaks"
    print(f"putting the peaks into {filename}...")
    with open(filename, "w") as f:
        f.write("".join(peaks))


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) != 1:
        raise Exception("Failed to get one argument - path to *.hashtree")
    putting_peaks(args[0], get_peaks(reading_hashtree(args[0])))
    print("all done!")
