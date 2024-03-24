import student_pb2 as Student


def from_student_to_cformat_data(student):
    def get_list_of_bytes_from_string(string):
        return list(map(lambda x: x.to_bytes(1, 'little'), string.encode()))

    name = get_list_of_bytes_from_string(student.name)
    login = get_list_of_bytes_from_string(student.login)
    group = get_list_of_bytes_from_string(student.group)

    practice = student.practice

    project_repo = get_list_of_bytes_from_string(student.project.repo)
    project_mark = student.project.mark
    
    mark = student.mark

    return [name, login, group, practice, project_repo, project_mark, mark]

def build_student_proto(processed_data):
    # Fill in student structure
    student = Student.Student()
    student.name = processed_data[0]
    student.login = processed_data[1]
    student.group = processed_data[2]
    student.practice.extend(processed_data[3])
    student.project.repo = processed_data[4]
    student.project.mark = processed_data[5]
    student.mark = processed_data[6]
    return student