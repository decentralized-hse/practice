#!/usr/bin/env python
import atheris
import io
import os
import sys

with atheris.instrument_imports():
  from json_zhukov_konstantinov import json_to_bin
  from json_zhukov_konstantinov import bin_to_json


atheris.instrument_func
def atheris_fuzzer(bin_in: bytearray):
  bin_file = 'test.bin'
  json_file = 'test.json'
  with open(bin_file, 'wb') as f:
    f.write(bin_in)
  code = os.system(f"./checker.o {bin_file} 2> out.log")
  exit_status = os.WEXITSTATUS(code)
  if exit_status != 0:
      return
  try:
    bin_to_json(bin_file, json_file)
    json_to_bin(json_file, bin_file)
  except ValueError as e:
    #print("FIND ERROR:",e)
    return
  with open(bin_file, 'rb') as f:
    bin_out = f.read()
  if bin_in == bin_out:
    return
  raise RuntimeError(f"mismatch\nin: {bin_in.hex()}\nout: {bin_out.hex()}\n")

atheris.instrument_func
def start_fuzz(data):
  bin_in = bytearray(data)
  atheris_fuzzer(bin_in)

def fuzz():
  atheris.Setup(sys.argv, start_fuzz)
  atheris.Fuzz()

def debug_main():
  input = ""
  bin_in = bytearray.fromhex(input)
  atheris_fuzzer(bin_in)

if __name__ == "__main__":
  os.system(f"gcc ../bin-gritzko-check.c -o ./checker.o")
  #debug_main()
  fuzz()