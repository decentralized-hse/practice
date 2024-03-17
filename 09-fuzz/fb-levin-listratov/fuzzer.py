#!/usr/bin/env python
import atheris
import io
import sys

with atheris.instrument_imports():
  from fb_levin.fb_levin import flat_to_bin
  from fb_levin.fb_levin import bin_to_flat

atheris.instrument_func
def check_roundtrip(bin_in: bytearray):
  bin_f = io.BytesIO(bin_in)
  bin_f.seek(0)
  flat_f = io.BytesIO()
  try:
    bin_to_flat(bin_f, flat_f)
  except ValueError as e:
    return
  flat_f.seek(0)
  bin_f = io.BytesIO()
  flat_to_bin(flat_f, bin_f)
  bin_f.seek(0)
  bin_out = bin_f.read()
  if bin_in == bin_out:
    return
  raise RuntimeError(f"mismatch\nin: {bin_in.hex()}\nout: {bin_out.hex()}\n")

atheris.instrument_func
def test_one_input(data):
  if len(data) != 128:
    return  # Input must be 4 byte integer.
  bin_in = bytearray(data)
  check_roundtrip(bin_in)

def fuzz():
  atheris.Setup(sys.argv, test_one_input)
  atheris.Fuzz()

def debug_main():
  input = ""
  bin_in = bytearray.fromhex(input)
  check_roundtrip(bin_in)

if __name__ == "__main__":
  #debug_main()
  fuzz()
