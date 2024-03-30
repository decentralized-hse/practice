import struct
import random
import string


def generate_random_string(length):
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))


def generate_random_url():
    return 'https://' + generate_random_string(55)

def generate_student():
    student_data = bytearray()
    student_data.extend(generate_random_string(32).encode('utf-8'))  
    student_data.extend(generate_random_string(16).encode('ascii'))
    student_data.extend(generate_random_string(8).encode('ascii'))
    student_data.extend(random.getrandbits(8).to_bytes(1, byteorder='little'))
    student_data.extend(generate_random_url().encode('utf-8'))
    student_data.extend(random.getrandbits(8).to_bytes(1, byteorder='little'))
    student_data.extend(struct.pack('<f', random.uniform(0, 100)))
    return student_data


def main():
    num_students = 10
    with open('students.bin', 'wb') as f:
        for _ in range(num_students):
            student_data = generate_student()
            f.write(student_data)


if __name__ == '__main__':
    main()
