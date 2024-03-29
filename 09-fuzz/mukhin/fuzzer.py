import subprocess

import frelatage
import os
from pathlib import Path


def target(input_path):
    input_data = None
    with open(input_path, 'rb') as f_in:
        input_data = f_in.read()

    file = Path("temp/input.bin")
    file.write_bytes(input_data)

    xml_name = input_path[:-3] + "xml"
    run = subprocess.run(["python3", "target.py", "temp/input.bin"])
    if run.stderr and run.stderr.startswith(b'Malformed input at'):
        return

    run = subprocess.run(["python3", "target.py", xml_name])
    if run.stderr and run.stderr.startswith(b'Malformed input at'):
        return

    with open(input_path, 'rb') as f_in:
        assert f_in.read() == input_data


def fuzz():
    input = frelatage.Input(file=True, value='ivanov.bin')
    fuzzer = frelatage.Fuzzer(target, [[input]], infinite_fuzz=False)
    fuzzer.fuzz()


if __name__ == '__main__':
    fuzz()
