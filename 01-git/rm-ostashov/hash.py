import argparse
from hashlib import sha256
import os
from checksumdir import dirhash

parser = argparse.ArgumentParser(description='Get the hash of an object by path')
parser.add_argument('path', help='Path to the object')
args = parser.parse_args()
path = args.path
if os.path.isdir(path):
    print(dirhash(path, 'sha256'))
else:
    f = open(path, 'r')
    bytesData = f.read()
    print(sha256(bytesData.encode()).hexdigest())