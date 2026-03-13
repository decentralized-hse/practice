import struct
import sys
import os

STRUCT_FMT = '<32s16s8s8B59sBf'
STRUCT_SIZE = struct.calcsize(STRUCT_FMT)


def _decode_cstr(raw: bytes) -> str:
    end = raw.find(b'\x00')
    return (raw[:end] if end != -1 else raw).decode('utf-8')


def _encode_cstr(s: str, length: int) -> bytes:
    enc = s.encode('utf-8')
    return enc + b'\x00' * (length - len(enc))


def unpack_student(raw: bytes) -> dict:
    f = struct.unpack(STRUCT_FMT, raw)
    return {
        'name':     _decode_cstr(f[0]),
        'login':    _decode_cstr(f[1]),
        'group':    _decode_cstr(f[2]),
        'practice': list(f[3:11]),
        'project':  {'repo': _decode_cstr(f[11]), 'mark': f[12]},
        'mark':     f[13],
    }


def pack_student(s: dict) -> bytes:
    return struct.pack(
        STRUCT_FMT,
        _encode_cstr(s['name'],  32),
        _encode_cstr(s['login'], 16),
        _encode_cstr(s['group'],  8),
        *s['practice'],
        _encode_cstr(s['project']['repo'], 59),
        int(s['project']['mark']),
        float(s['mark']),
    )


def _rdx_escape(s: str) -> str:
    s = s.replace('\\', '\\\\').replace('"', '\\"')
    return f'"{s}"'


def student_to_rdx(s: dict) -> str:
    practice = '[' + ' '.join(str(p) for p in s['practice']) + ']'
    project = f'({_rdx_escape(s["project"]["repo"])} {s["project"]["mark"]})'
    return (
        '{'
        f'name:{_rdx_escape(s["name"])} '
        f'login:{_rdx_escape(s["login"])} '
        f'group:{_rdx_escape(s["group"])} '
        f'practice:{practice} '
        f'project:{project} '
        f'mark:{repr(s["mark"])}'
        '}'
    )


def students_to_rdx(students: list) -> str:
    body = '\n'.join(student_to_rdx(s) for s in students)
    return f'[\n{body}\n]\n'


class _RDXParser:
    def __init__(self, text: str):
        self._t = text
        self._i = 0

    def _skip_ws(self):
        while self._i < len(self._t) and self._t[self._i] in ' \t\n\r':
            self._i += 1

    def _peek(self) -> str:
        self._skip_ws()
        return self._t[self._i] if self._i < len(self._t) else ''

    def _eat(self, ch: str):
        self._skip_ws()
        self._i += 1

    def _string(self) -> str:
        self._eat('"')
        out = []
        ESC = {'n': '\n', 'r': '\r', 't': '\t', '"': '"', '\\': '\\'}
        while self._i < len(self._t):
            c = self._t[self._i]
            if c == '"':
                self._i += 1
                return ''.join(out)
            if c == '\\':
                self._i += 1
                e = self._t[self._i]; self._i += 1
                out.append(ESC.get(e, e))
            else:
                out.append(c); self._i += 1

    def _number(self):
        self._skip_ws()
        start = self._i
        if self._i < len(self._t) and self._t[self._i] == '-':
            self._i += 1
        while self._i < len(self._t) and self._t[self._i] in '0123456789.eE+':
            self._i += 1
        tok = self._t[start:self._i]
        return float(tok) if ('.' in tok or 'e' in tok or 'E' in tok) else int(tok)

    def _practice(self) -> list:
        self._eat('[')
        vals = []
        while self._peek() != ']':
            vals.append(int(self._number()))
        self._eat(']')
        return vals

    def _project(self) -> dict:
        self._eat('(')
        repo = self._string()
        mark = int(self._number())
        self._eat(')')
        return {'repo': repo, 'mark': mark}

    def _ident(self) -> str:
        self._skip_ws()
        start = self._i
        while self._i < len(self._t) and self._t[self._i] not in ': \t\n\r{}[]()':
            self._i += 1
        return self._t[start:self._i]

    def _student(self) -> dict:
        self._eat('{')
        s: dict = {}
        while self._peek() != '}':
            key = self._ident()
            self._eat(':')
            if key in ('name', 'login', 'group'):
                s[key] = self._string()
            elif key == 'practice':
                s[key] = self._practice()
            elif key == 'project':
                s[key] = self._project()
            elif key == 'mark':
                s[key] = float(self._number())
        self._eat('}')
        return s

    def parse(self) -> list:
        students = []
        self._eat('[')
        while self._peek() != ']':
            students.append(self._student())
        self._eat(']')
        return students


def parse_rdx(text: str) -> list:
    return _RDXParser(text).parse()


def read_bin(path: str) -> list:
    raw = open(path, 'rb').read()
    n = len(raw) // STRUCT_SIZE
    return [unpack_student(raw[i * STRUCT_SIZE:(i + 1) * STRUCT_SIZE]) for i in range(n)]


def write_bin(path: str, students: list):
    with open(path, 'wb') as f:
        for s in students:
            f.write(pack_student(s))


def read_rdx(path: str) -> list:
    return parse_rdx(open(path, encoding='utf-8').read())


def write_rdx(path: str, students: list):
    open(path, 'w', encoding='utf-8').write(students_to_rdx(students))


def main():
    path = sys.argv[1]
    base, ext = os.path.splitext(path)
    if ext.lower() == '.bin':
        students = read_bin(path)
        out = base + '.rdx'
        write_rdx(out, students)
        print(f"written to {out}")
    else:
        students = read_rdx(path)
        out = base + '.bin'
        write_bin(out, students)
        print(f"written to {out}")


if __name__ == '__main__':
    main()
