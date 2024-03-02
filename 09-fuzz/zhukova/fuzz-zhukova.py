import os
import sys
import frelatage

LOG_FILE = "out.log"
FILE_NAME = "test"
PROG = "kv-balobanov-zhukova.py"

def prapare():
    os.system(f"gcc ../bin-gritzko-check.c -o ./checker.o")

def validate_input(file_name: str):
    code = os.system(f"./checker.o {file_name} 2> {LOG_FILE}")
    exit_status = os.WEXITSTATUS(code)
    return exit_status == 0

def encode_data(file_name: str):
    print('! encoding: .kv -> .bin', file=sys.stderr)
    code = os.system(f"python3 {PROG} {file_name} > {LOG_FILE}")
    exit_status = os.WEXITSTATUS(code)
    assert exit_status == 0

def decode_data(file_name: str):
    print('! decoding: .bin -> .kv', file=sys.stderr)
    code = os.system(f"python3 {PROG} {file_name} > {LOG_FILE}")
    exit_status = os.WEXITSTATUS(code)
    assert exit_status == 0

def kv_parser_round_trip_executor(data):
    bin_fname = f"{FILE_NAME}.bin"
    kv_fname = f"{FILE_NAME}.kv"
    with open(bin_fname, "w") as f:
        f.write(data)
    if not validate_input(bin_fname):
        return
    try:
        decode_data(bin_fname)
        encode_data(kv_fname)
    except:
        os._exit(1)
    with open(bin_fname, "r") as f:
        if f.read() != data:
            with open("sample", "w") as f:
                f.write(data + "\nvs\n" + f.read())
            os._exit(1)

prapare()
input = frelatage.Input(value="initial_value")
f = frelatage.Fuzzer(kv_parser_round_trip_executor, [[input]])
f.fuzz()