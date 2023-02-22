import os
import schema
import struct
import sys



def get_file_type(file_name):
    return file_name.split('.')[-1]

path = sys.argv[1]
file_type = get_file_type(path)
if file_type == ".flat":
    fb_to_bin(sys.argv[1])
elif file_type == ".bin":
    bin_to_fb(sys.argv[1])
else:
    print("Bad format!")
