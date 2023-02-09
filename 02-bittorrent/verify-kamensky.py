#!/bin/python3

import argparse
import hashlib
from pathlib import Path


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("file_path", type=str)
    parser.add_argument("chunk_idx", type=int)
    return parser.parse_args()


def get_file_peaks(file_path):
    return Path(f'{file_path}.peaks').read_text()


def get_file_hash(file_path):
    return Path(f'{file_path}.root').read_text()


def sha256_hash(data):
    return hashlib.sha256(data.encode()).hexdigest()


def verify_file_peaks(file_path):
    peaks = get_file_peaks(file_path)
    root = get_file_hash(file_path)
    calculated_root = sha256_hash(peaks)
    return calculated_root == root.strip()


def get_chunk_hash(file_path, chunk_idx):
    return sha256_hash(Path(f'{file_path}.{chunk_idx}.chunk').read_text())


def get_peaks(file_path):
    return Path(f'{file_path}.peaks').read_text().splitlines()


def get_uncles(file_path, chunk_idx):
    return Path(f'{file_path}.{chunk_idx}.proof').read_text().splitlines()


def get_peak_high(peaks, chunk_idx):
    tmp = 0
    for i in range(len(peaks) - 1, -1, -1):
        if peaks[i] != "0" * 64:
            tmp += 1 << i
            if tmp > chunk_idx:
                return i, tmp


def verify_chunk(file_path, chunk_idx):
    curr_hash = get_chunk_hash(file_path, chunk_idx)
    print(f'Current chunk hash: {curr_hash}')

    peaks = get_peaks(file_path)
    high, cnt = get_peak_high(peaks, chunk_idx)
    idx =  (1 << high) + chunk_idx - cnt

    uncles = get_uncles(file_path, chunk_idx)
    for i in range(0, high):
        if idx % 2:
            curr_hash = sha256_hash(uncles[i] + '\n' + curr_hash + '\n')
        else:
            curr_hash = sha256_hash(curr_hash + '\n' + uncles[i] + '\n')
        idx = idx // 2
        curr_peak = f'peak {i + 1}: {peaks[i + 1]}'
        if peaks[i + 1] != curr_hash:
            print(f'Failed on {curr_peak}')
            return False
        print(f'Matched with {curr_peak}')
    return True


if __name__ == "__main__":
    args = get_args()
    if not verify_file_peaks(args.file_path):
        print("Incorrect peaks!")
        exit(1)
    print("Peaks verified!")

    if not verify_chunk(args.file_path, args.chunk_idx):
        print("Failed!")
        exit(1)
    print("Done!")
