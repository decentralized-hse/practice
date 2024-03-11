#!/bin/python3

from dicttoxml import dicttoxml
from pathlib import Path
from xml.dom.minidom import parseString
import argparse
import struct
import xmltodict
import sys


def validate_bytes_structure(bytes):
    if len(bytes) % 128 != 0:
        return False
    return True


def XmlEncode(object):
    return parseString(dicttoxml(object)).toprettyxml()


def parceTypes(tmpDict):
    currType = tmpDict['@type']
    if currType == 'int':
        return int(tmpDict['#text'])
    elif currType == 'float':
        return float(tmpDict['#text'])
    elif currType == 'str':
        return tmpDict['#text']
    elif currType == 'dict':
        newDict = dict()
        for dictItem in tmpDict.keys():
            if dictItem == '@type':
                continue
            newDict[dictItem] = parceTypes(tmpDict[dictItem])
        return newDict
    elif currType == 'list':
        lst = list()
        if type(tmpDict['item']) is dict:
            tmpDict['item'] = [tmpDict['item']]
        for listItem in tmpDict['item']:
            lst.append(parceTypes(listItem))
        return lst


def XmlDecode(fileName):
    try:
        file = open(fileName, 'rb')
        tmpObject = xmltodict.parse(file, dict_constructor=dict)['root']
        newObject = dict()
        for key in tmpObject:
            newObject[key] = parceTypes(tmpObject[key])
        return newObject
    except Exception as e:
        sys.stderr.write("Malformed input\n")
        sys.exit(-1)


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
        lst[i] = DeserializeUint8(bytes[i:i + 1])
    return lst


def SerializeString(obj, size):
    ans = obj.encode()[:size]
    ans += b'\x00' * (size - len(ans))
    return ans


def DeserializeString(bytes):
    return bytes.decode().rstrip('\x00')


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("file_path", type=str)
    return parser.parse_args()


def BytesToObject(bytes):
    if not validate_bytes_structure(bytes):
        sys.stderr.write("Malformed input\n")
        sys.exit(-1)

    ans = {
        'data': []
    }
    for i in range(0, len(bytes), 128):
        ans['data'].append({
            'name': DeserializeString(bytes[i:i + 32]),
            'login': DeserializeString(bytes[i + 32:i + 48]),
            'group': DeserializeString(bytes[i + 48:i + 56]),
            'practice': DeserializeListOfUint8(bytes[i + 56:i + 64]),
            'project': {
                'repo': DeserializeString(bytes[i + 64:i + 123]),
                'mark': DeserializeUint8(bytes[i + 123:i + 124])
            },
            'mark': DeserializeFloat(bytes[i + 124:i + 128]),
        })
    cnt = len(ans["data"])
    print(f'{cnt} student{"s" if cnt > 1 else ""} found')
    print(ans)
    return ans


def process_bin(file_path):
    try:
        print(f'Processing file {file_path}')
        bytes = Path(file_path).read_bytes()
    except IOError:
        sys.stderr.write("Malformed input\n")
        sys.exit(-1)

    if not validate_bytes_structure(bytes):
        sys.stderr.write("Malformed input\n")
        sys.exit(-1)

    serializedObject = XmlEncode(BytesToObject(bytes))
    output_filename = f'{file_path[:-4]}.xml'
    XmlSaveEncoded(serializedObject, output_filename)
    print(f'Xml data written to {output_filename}')


def ObjectToBytes(object):
    cnt = len(object["data"])
    print(f'{cnt} student{"s" if cnt > 1 else ""} found')
    ans = b''
    for item in object['data']:
        ans += SerializeString(item['name'], 32)
        ans += SerializeString(item['login'], 16)
        ans += SerializeString(item['group'], 8)
        ans += SerializeListOfUint8(item['practice'], 8)
        ans += SerializeString(item['project']['repo'], 59)
        ans += SerializeUint8(item['project']['mark'])
        ans += SerializeFloat(item['mark'])
    return ans


def process_xml(file_path):
    print(f'Processing file {file_path}')
    object = XmlDecode(file_path)
    file = Path(f'{file_path[:-4]}.bin')
    file.write_bytes(ObjectToBytes(object))
    print(f'Data written to {file.name}')


if __name__ == "__main__":
    args = get_args()
    if args.file_path.endswith('.bin'):
        process_bin(args.file_path)
    elif args.file_path.endswith('.xml'):
        process_xml(args.file_path)
    print("Done!")

"""
решение не выполняло условие:

решение должно отклонять любой ввод, не соответствующий формату 
(выход с кодом -1 и сообщением Malformed input в stderr)

так же решение падало из-за этого на некоторых тестах
"""
