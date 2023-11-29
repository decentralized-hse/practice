#!/bin/python3
from typing import Any

from dicttoxml import dicttoxml
import xml.etree.ElementTree as ElementTree
from pathlib import Path
from xml.dom.minidom import parseString
import argparse
import struct


def XmlEncode(object):
    return parseString(dicttoxml(object)).toprettyxml()


def parse_element(element: ElementTree.Element):
    element_type = element.attrib['type']
    if element_type == 'int':
        return int(element.text)
    elif element_type == 'float':
        return float(element.text)
    elif element_type == 'str':
        return element.text if element.text else ''
    elif element_type == 'dict':
        return {child.tag: parse_element(child) for child in list(element)}
    elif element_type == 'list':
        return list(map(parse_element, list(element)))


def XmlDecode(file_name: str):
    data = ElementTree.parse(file_name).getroot()[0]
    return parse_element(data)


def XmlSaveEncoded(serializedObject, fileName):
    Path(fileName).write_text(serializedObject)


def SerializeUint8(obj):
    return obj.to_bytes(1, 'little')


def DeserializeUint8(bytes):
    return int.from_bytes(bytes, 'little')


def SerializeFloat(obj):
    return struct.pack("<f", obj)


def DeserializeFloat(bytes):
    return struct.unpack("<f", bytes)[0]


def SerializeListOfUint8(obj, size):
    obj = obj[:size]
    for _ in range(size - len(obj)):
        obj.append(0)
    return b''.join(map(lambda x: SerializeUint8(x), obj))


def DeserializeListOfUint8(bytes):
    lst = [0] * len(bytes)
    for i in range(len(bytes)):
        lst[i] = DeserializeUint8(bytes[i:i+1])
    return lst


def SerializeString(obj, size):
    ans = obj.encode('utf-8')[:size]
    ans += b'\x00' * (size - len(ans))
    return ans


def DeserializeString(bytes):
    return bytes.decode().rstrip('\x00')


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("file_path", type=str)
    return parser.parse_args()


def BytesToObject(bytes):
    ans = {
        'data': []
    }
    for i in range(0, len(bytes), 128):
        ans['data'].append({
            'name': DeserializeString(bytes[i:i+32]),
            'login': DeserializeString(bytes[i+32:i+48]),
            'group': DeserializeString(bytes[i+48:i+56]),
            'practice': DeserializeListOfUint8(bytes[i+56:i+64]),
            'project': {
                'repo': DeserializeString(bytes[i+64:i+123]),
                'mark': DeserializeUint8(bytes[i+123:i+124])
            },
            'mark': DeserializeFloat(bytes[i+124:i+128]),
        })
    cnt = len(ans["data"])
    print(f'{cnt} student{"s" if cnt > 1 else ""} found')
    return ans


def process_bin(file_path):
    print(f'Processing file {file_path}')
    bytes = Path(file_path).read_bytes()
    serializedObject = XmlEncode(BytesToObject(bytes))
    output_filename = f'{file_path[:-4]}.xml'
    XmlSaveEncoded(serializedObject, output_filename)
    print(f'Xml data written to {output_filename}')


def students_to_bytes(students: list[dict[str, Any]]) -> bytes:
    students_count = len(students)
    print(f'{students_count} student{"s" if students_count > 1 else ""} found')
    return b''.join(map(student_to_bytes, students))

def student_to_bytes(student: dict[str, Any]) -> bytes:
    student_bytes = b''
    student_bytes += SerializeString(student['name'], 32)
    student_bytes += SerializeString(student['login'], 16)
    student_bytes += SerializeString(student['group'], 8)
    student_bytes += SerializeListOfUint8(student['practice'], 8)
    student_bytes += SerializeString(student['project']['repo'], 59)
    student_bytes += SerializeUint8(student['project']['mark'])
    student_bytes += SerializeFloat(student['mark'])
    return student_bytes

def process_xml(file_path):
    print(f'Processing file {file_path}')
    object = XmlDecode(file_path)
    file = Path(f'{file_path[:-4]}.bin')
    file.write_bytes(students_to_bytes(object))
    print(f'Data written to {file.name}')


if __name__ == "__main__":
    args = get_args()
    if args.file_path.endswith('.bin'):
        process_bin(args.file_path)
    elif args.file_path.endswith('.xml'):
        process_xml(args.file_path)
    print("Done!")
