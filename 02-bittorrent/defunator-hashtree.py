from math import log2

import hashlib
import argparse

BLOCK_SIZE = 1024
HASH = lambda x : hashlib.sha256(x).hexdigest()


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=str)
    args = parser.parse_args()

    input_filename = args.input

    with open(input_filename, "rb") as f:
        bytes = f.read()

    if len(bytes) % BLOCK_SIZE != 0:
        print(f"Failed to chunk file into blocks: {len(bytes)} % {BLOCK_SIZE} != 0")
        exit(0)

    leave_hashes = []
    for start_index in range(0, len(bytes), BLOCK_SIZE):
        end_index = start_index + BLOCK_SIZE
        leave_hashes.append(HASH(bytes[start_index:end_index]))

    hash_tree_size = len(leave_hashes) * 2 - 1
    hash_tree = ["0" * hashlib.sha256().block_size] * hash_tree_size

    for i in range(len(leave_hashes)):
        hash_tree[2 * i] = leave_hashes[i]

    max_level = int(log2(len(leave_hashes)))
    cur_level_nodes = len(leave_hashes)
    for level in range(1, max_level + 1):
        cur_level_nodes //= 2
        for node_index in range(cur_level_nodes):
            hash_tree_index = 2 ** (level + 1) * node_index + 2 ** level - 1
            left_hash, right_hash = hash_tree[hash_tree_index - 2 ** (level - 1)], hash_tree[hash_tree_index + 2 ** (level - 1)]
            hash_tree[hash_tree_index] = HASH(f"{left_hash}\n{right_hash}\n".encode("UTF-8"))

    with open(f"{input_filename}.hashtree", "w") as f:
        f.writelines("\n".join(hash_tree))
