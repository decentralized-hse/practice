import sys
import os
import hashlib

def handle_no_file(name):
    print(f"Error: file or directory {name} doesn't exist")

def handle_bad_hash(name, correct):
    print(f"Error: bad hash for {name}")
    print(f"Hash should be equal {correct}")

def findDir(name_to_find, hash):
    if name_to_find == '.':
        return hash
    with open(hash, 'r') as f:
        text = f.read()
        sub_content = text.split('\n')
        for content in sub_content:
            c = content.split('/\t')
            if len(c) == 2:
                name, hash = c[0], c[1]
                if name == name_to_find:
                    return hash
                d = findDir(name, hash)
                if d != '':
                    return d
        return ''


def validateTree(hash):
    with open(hash, 'r') as f:
        text = f.read()
        sub_content = text.split('\n')
        text = text.encode('utf-8')
        if hash != hashlib.sha256(text).hexdigest():
            handle_bad_hash(hash, hashlib.sha256(text).hexdigest())
            return False


        for content in sub_content:
            if len(content) == 0:
                continue
            c = content.split(':\t')
            if len(c) == 2:
                # process file
                name, hash = c[0], c[1]
                # check if file exists
                if not os.path.exists(hash):
                    handle_no_file(hash)
                    return False
                # check content hash
                with open(hash, 'r') as simple_file:
                    contentHash = hashlib.sha256(simple_file.read().encode('utf-8')).hexdigest()
                if contentHash != hash:
                    handle_bad_hash(hash, contentHash)
                    return False
                continue

            c = content.split('/\t')
            if len(c) == 2:
                # process folder
                name, hash = c[0], c[1]
                 # check if directory file exists
                if not os.path.exists(hash):
                    handle_no_file(hash)
                    return False
                res = validateTree(hash)
                if not res:
                    return res
                continue
            print(c)
            # should not reach this
            return False
        return True


def main():
    args = sys.argv[1:]
    print(*args)

    hash = findDir(args[0], args[1])
    if hash == '':
        print('no such dir:', args[0])
        return
    print('hash of requested dir:', hash)
    if validateTree(hash):
        print('tree is valid')
    else:
        print('tree is not valid')

if __name__ == "__main__":
    main()
