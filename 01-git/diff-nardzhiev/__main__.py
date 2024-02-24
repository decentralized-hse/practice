import sys
import os

def input_validation():
    if len(sys.argv) != 4:
        raise Exception('Incorrect number of arguments')


def get_args_from_sys():
    input_validation()
    return sys.argv[1], sys.argv[2], sys.argv[3]


def try_parse_line_as_file(line):
    result = line.split(':\t', 1)
    if len(result) == 2:
        return result
    return None


def try_parse_line_as_dir(line):
    result = line.split('/\t', 1)
    if len(result) == 2:
        return result
    return None


def line_to_name_n_hash(line):
    name_n_hash = try_parse_line_as_file(line)
    if name_n_hash:
        is_dir = False
        return name_n_hash[0], name_n_hash[1], is_dir

    name_n_hash = try_parse_line_as_dir(line)
    if name_n_hash:
        is_dir = True
        return name_n_hash[0], name_n_hash[1], is_dir

    raise Exception('Unseparable string')


def hash_info_to_map(hash):
    # name -> hash
    objects_hash = dict()

    # name -> (0 - if dir, 1 else)
    objects_types = dict()

    with open(hash, 'r') as f:
        lines = [line.strip() for line in f]
        for line in lines:
            if len(line) == 0:
                continue
            obj_name, obj_hash, is_dir = line_to_name_n_hash(line)
            objects_hash[obj_name] = obj_hash
            objects_types[obj_name] = is_dir

    return objects_hash, objects_types


def get_diff(path, old_hash, new_hash):
    diff = ""
    prev_objects_hash, prev_is_dir = hash_info_to_map(old_hash)
    cur_objects_hash, cur_is_dir = hash_info_to_map(new_hash)

    for prev_obj, prev_hash in prev_objects_hash.values():
        if prev_is_dir[prev_obj]:
            continue
        if prev_obj in cur_objects_hash and cur_is_dir[prev_obj]:
            continue
        if (prev_obj not in cur_objects_hash) or (prev_obj in cur_objects_hash and prev_hash != cur_objects_hash[prev_obj]):
            diff += f"- {os.path.join(path, prev_obj)}\n"
        if prev_obj in cur_objects_hash:
            del cur_objects_hash[prev_obj]

    for cur_obj in cur_objects_hash.keys():
        if cur_is_dir[cur_obj]:
            continue
        diff += f"+ {os.path.join(path, cur_obj)}\n"

    for prev_obj in prev_objects_hash.keys():
        if not prev_is_dir[prev_obj]:
            continue
        if not cur_is_dir[prev_obj]:
            continue
        nested_diff = get_diff(os.path.join(path, prev_obj), prev_objects_hash[prev_obj], cur_objects_hash[prev_obj])
        if len(nested_diff) == 0:
            continue
        diff += f"d {os.path.join(path, prev_obj)}\n"
        diff += nested_diff

    return diff


if __name__ == '__main__':
    path, old_hash, new_hash = get_args_from_sys()
    diff = get_diff(path=path, old_hash=old_hash, new_hash=new_hash)
    print(diff)
