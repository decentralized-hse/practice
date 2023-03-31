import capnp
import struct
import sys

fmt = "<32s16s8s8s59sBf"
struct_size = struct.calcsize(fmt)
student_capnp = capnp.load('student.capnp')


def BinToCapnp(filename_input, filename_output):
    with open(filename_input, 'rb') as f:
        file_data = f.read()
    id = 0
    len_data = len(file_data)
    g = open(filename_output, 'ab')
    # Очистка файла
    g.truncate(0)
    while len_data >= struct_size:
        while file_data[id] == ord('\n'):
          id += 1
        student_bin = struct.unpack(fmt, file_data[id:id+struct_size])
        id += struct_size
        len_data -= struct_size
        student = student_capnp.Student.new_message()
        a = student.init('practice', 8)
        project = student_capnp.Student.Project.new_message()
        student.name = student_bin[0].decode().strip('\x00')
        student.login = student_bin[1].decode().strip('\x00')
        student.group = student_bin[2].decode().strip('\x00')
        p = ""
        for i in student_bin[3]:
            p += str(i)
        for i in range(8):
            a[i] = int(p[i])
        project.repo = student_bin[4].decode().strip('\x00')
        project.mark = student_bin[5]
        student.project = project
        student.mark = student_bin[6]
        student.write(g)
    g.close()


def add_empty_symbol(s: str, count: int) -> bytes:
    s = s.encode()
    len_s = len(s)
    if len_s < count:
        s += b'\x00' * (count - len_s)
    return s


def CapnpToBin(filename_input: str, filename_output: str) -> None:
    input = open(filename_input, 'r+b')
    f = open(filename_output, 'ab')
    # Очистка файла
    f.truncate(0)
    while 1:
        try:
            student = student_capnp.Student.read(input)
        except Exception:
            break
        name = add_empty_symbol(student.name, 32)
        login = add_empty_symbol(student.login, 16)
        group = add_empty_symbol(student.group, 8)
        practice = str(student.practice)
        p = ""
        for i in practice:
            if i == '0':
                p += '\x00'
            if i == '1':
                p += '\x01'
        repo = add_empty_symbol(student.project.repo, 59)
        mark = student.project.mark
        mark_all = student.mark
        record = struct.pack(fmt, name, login, group, p.encode(), repo, mark, mark_all)
        f.write(record)


if __name__ == '__main__':
    filename_input = sys.argv[1]
    filename_output = sys.argv[2]
    if filename_input.endswith('.bin'):
        BinToCapnp(filename_input, filename_output)
    else:
        CapnpToBin(filename_input, filename_output)
