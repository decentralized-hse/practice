from main import BinToCapnp, CapnpToBin

import subprocess

import frelatage


def target(input_path):
    stderr = subprocess.run(["./validator", input_path], capture_output=True).stderr

    if stderr.startswith(b'Malformed input at'):
        return

    temp_path = f'temp/temp'
    output_path = f'temp/output'

    try:
        BinToCapnp(input_path, temp_path)
        CapnpToBin(temp_path, output_path)
    except RuntimeError:
        return -1

    with open(input_path, 'rb') as f_in:
        with open(output_path, 'rb') as f_out:
            assert f_in.read() == f_out.read()


def fuzz():
    input = frelatage.Input(file=True, value='ivanov.bin')
    fuzzer = frelatage.Fuzzer(target, [[input]], infinite_fuzz=True)
    fuzzer.fuzz()


if __name__ == '__main__':
    # target('out/id:000000,err:assertionerror,err_file:fuzzing,err_pos:26/0/ivanov.bin')
    fuzz()
