import atheris
import sqlite3
import os
import sys

with atheris.instrument_imports():
  from sqlite_savin_polezhaev import SqliteToBin
  from sqlite_savin_polezhaev import BinToSqlite

FILE_NAME = "promegut"
LOG_FILE = "checker.log"

def check_binary(file_name: str):
    code = os.system(f"./checker.o {file_name} 2> {LOG_FILE}")
    exit_status = os.WEXITSTATUS(code)
    return exit_status == 0

atheris.instrument_func
def check_roundtrip(bin_in: bytearray):
  bin_f = FILE_NAME + ".bin"
  sqlite_f = FILE_NAME + ".db"
  
  with open(bin_f, "wb") as f:
    f.write(bin_in)
  
  if not check_binary(bin_f):
    return
  
  try:
    BinToSqlite(bin_f)
  except ValueError as e:
    return
  
  SqliteToBin(sqlite_f)
  
  conn = sqlite3.connect(sqlite_f)
  cursor = conn.cursor()
  cursor.execute('DROP TABLE students')
  conn.commit()
  conn.close()
  
  with open(bin_f, "rb") as f:
    bin_out = f.read()
  
  if bin_in == bin_out:
    return
  raise RuntimeError(f"Error with\ninput: {bin_in.hex()}\noutput: {bin_out.hex()}\n")

atheris.instrument_func
def test_one_input(data):
  if len(data) != 128:
    return
  bin_in = bytearray(data)
  check_roundtrip(bin_in)

def fuzz():
  os.system(f"gcc ../bin-gritzko-check.c -o ./checker.o")
  atheris.Setup(sys.argv, test_one_input)
  atheris.Fuzz()

if __name__ == "__main__":
  fuzz()