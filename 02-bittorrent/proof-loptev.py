from argparse import ArgumentParser
from pathlib import Path

def read_hashes(filepath):
    with open(f"{filepath}.hashtree", "r") as f:
        hashes = []
        for line in f:
            hashes.extend(line.strip().split())
        return hashes

def get_mask_size(mask):
    return bin(mask).count("1") + 1

assert get_mask_size(0b0111) == 4

def sibling(node, mask):
    return node ^ (1 << get_mask_size(mask))

assert sibling(0b00100, 0b0) == 0b00110

def parent_and_new_mask(node, mask):
    mask = (mask << 1) + 1
    mask_size = get_mask_size(mask)
    return ((node >> mask_size) << mask_size) | mask, mask

assert parent_and_new_mask(0b00100, 0b0) == (0b00101, 0b01)
assert parent_and_new_mask(0b01110, 0b0) == (0b01101, 0b01)

def right_subtree_bound(node, mask):
    tree_size = (1 << get_mask_size(mask)) - 1
    return node + (tree_size // 2)

assert right_subtree_bound(0b10011, 0b011) == 22

def is_right(node, mask):
    return node & (1 << get_mask_size(mask)) != 0

assert is_right(0b00100, 0b0) == False
assert is_right(0b01110, 0b0) == True

def prove(hashes, package_number):
    node = package_number * 2
    proof = []
    mask = 0
    while is_right(node, mask) or right_subtree_bound(sibling(node, mask), mask) < len(hashes):
        proof.append(hashes[sibling(node, mask)])
        node, mask = parent_and_new_mask(node, mask)
    return proof

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument(
        "filepath", type=Path, help="Path to the file containing original data"
    )
    parser.add_argument(
        "package_number",
        type=int,
        help="The number of the package for which the proof is requested",
    )
    args = parser.parse_args()
    hashes = read_hashes(args.filepath)
    proof = prove(hashes, args.package_number)
    with open(f"{args.filepath}.{args.package_number}.proof", "w") as fout:
        if proof:
            print(*proof, sep=' ', file=fout)
