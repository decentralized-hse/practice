import sqlite3
import struct
import sys

def BinToSqlite(filename):
    fmt = "<32s16s8s8s59sBf"
    struct_size = struct.calcsize(fmt)

    with open(filename, 'rb') as f:
        bin_data = f.read()

    for i in range(0, len(bin_data), struct_size):
        data = []
        student = struct.unpack(fmt, bin_data[i:i+struct_size])
        name = student[0].decode('utf8').strip('\x00')
        name += '\x00' * (32 - len(name))
        login = student[1].decode('utf-8').strip('\x00')
        group = student[2].decode('utf-8').strip('\x00')
        practice = ''
        for i in student[3]:
            practice += str(i)
            practice += ','
        practice = practice[0:-1]
        repo = student[4].decode('utf-8').strip('\x00')
        mark = student[5]
        mark_float = student[6]
        project = {
            'repo': repo,
            'mark': mark
        }
        data.append({
            'name': name,
            'login': login,
            'group': group,
            'practice': practice,
            'project': project,
            'mark': mark_float
        })
        conn = sqlite3.connect('students.db')
        cursor = conn.cursor()
        cursor.execute('CREATE TABLE IF NOT EXISTS students ('
                    'id INTEGER PRIMARY KEY AUTOINCREMENT, '
                    'name TEXT, '
                    'login TEXT, '
                    '"group" TEXT, '
                    'practice BLOB, '
                    'repo TEXT, '
                    'mark INTEGER, '
                    'mark_float REAL)')

        for student in data:
            cursor.execute('INSERT INTO students (name, login, "group", practice, repo, mark, mark_float) '
                        'VALUES (?, ?, ?, ?, ?, ?, ?)',
                        (student['name'], student['login'], student['group'], student['practice'],
                            student['project']['repo'], student['project']['mark'], student['mark']))

        conn.commit()
        conn.close()

def SqliteToBin(filename):


    conn = sqlite3.connect(filename)
    cursor = conn.cursor()
    cursor.execute('SELECT * FROM students')
    students = cursor.fetchall()

    fmt = '32s16s8s8s59sBf'

    with open('students.bin', 'wb') as f:
        for student in students:
            name = student[1].encode()
            if len(name) < 32:
                name += b'\x00' * (32 - len(name))

            login = student[2].encode()
            group = student[3].encode()
            tmp = student[4].split(',')
            practice = []
            for i in tmp:
                practice.append(int(i))
            practice = bytes(practice)
            project_repo = student[5].encode()
            record = struct.pack(fmt, name, login, group, practice, project_repo, student[6], student[7])
            f.write(record)
        
    conn.close()

if __name__ == '__main__':
    print('Start')
    filename = sys.argv[1]
    if filename.endswith('.bin'):
        print('Creating .db file')
        BinToSqlite(filename)
    else:
        print('Creating .bin file')
        SqliteToBin(filename)
    print('End')