#!/usr/bin/env python
import atheris
import io
import sys

with atheris.instrument_imports():
  from protobuf_vernigor import bin_to_protobuf, protobuf_to_bin, get_output_file_path
  from consts import BIN_EXTENSION, PROTOBUF_EXTENSION

atheris.instrument_func
def check_roundtrip(bin_in: bytearray):
  binary = 'input.bin'
  with open(binary, 'wb') as f:
    f.write(bin_in)
  try:
    bin_to_protobuf(binary)
  except ValueError as e:
    return
  
  protobuf = get_output_file_path(binary, PROTOBUF_EXTENSION)
  protobuf_to_bin(protobuf)

  bin_f = get_output_file_path(protobuf, BIN_EXTENSION)
  with open(bin_f, 'rb') as f:
    bin_out = f.read()
  if bin_in == bin_out:
    return
  raise RuntimeError(f"mismatch\nin: {bin_in.hex()}\nout: {bin_out.hex()}\n")

atheris.instrument_func
def test_one_input(data):
  if len(data) != 128:
    return  # Input must be 4 byte integer.
  bin_in = bytearray(data)
  check_roundtrip(bin_in)


if __name__ == "__main__":
  atheris.Setup(sys.argv, test_one_input)
  atheris.Fuzz()

