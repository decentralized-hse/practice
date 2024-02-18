import os
import sys
from hashlib import sha256

NEW_LINE_SYMBOL = '\n'
class CommandPut:
    def __init__(self):
        self.path_to_file_from_input = sys.argv[1]
        self.hash_value = sys.argv[2]

    def get_current_hash_from_data_and_write_file(self, data):
        encoded_data = data.encode()
        hash_value = sha256(encoded_data).hexdigest()
        with open(hash_value, 'wb') as file:
            file.write(encoded_data)
        return hash_value
    def update_directories(self, directories_to_update, indexes_to_update):
        current_hash = self.get_current_hash_from_data_and_write_file(''.join(self.input_data))
        for i in range(len(directories_to_update) - 1, -1, -1):
            lines_list = [line for line in directories_to_update[i].split(NEW_LINE_SYMBOL) if len(line) > 1]
            current_index_to_update = indexes_to_update[i]
            if current_index_to_update == len(lines_list):
                lines_list.append(f"{self.name_of_file}:\t{current_hash}")
            else:
                lines_list[current_index_to_update] = lines_list[current_index_to_update][:-64] + current_hash
            lines_list.sort()
            content_for_directory = NEW_LINE_SYMBOL.join(lines_list) + NEW_LINE_SYMBOL
            current_hash = self.get_current_hash_from_data_and_write_file(content_for_directory)
        return current_hash

    def get_directories_and_indexes_to_update(self, given_hash):
        directories_to_update = []
        names_list = [name for name in self.parsed_path_from_input[0].split(os.sep)]
        indexes_to_update = []
        current_hash = given_hash
        for i in range(len(names_list) - 1):
            current_name = names_list[i]
            content = self.read_file_by_hash(current_hash)
            list_of_lines = [line for line in content.split(NEW_LINE_SYMBOL) if len(line) > 1]
            hash_value, index = self.find_hash_and_index(current_name, list_of_lines, True)
            if hash_value:
                indexes_to_update.append(index)
                directories_to_update.append(content)
                current_hash = hash_value
            else:
                print(f"There is no folder with name={current_name}")
                sys.exit(-1)

        closest_folder = self.read_file_by_hash(current_hash)
        directories_to_update.append(closest_folder)
        list_of_lines = [line for line in closest_folder.split(NEW_LINE_SYMBOL) if len(line) > 1]
        _, index = self.find_hash_and_index(self.name_of_file, list_of_lines)
        indexes_to_update.append(index)
        return directories_to_update, indexes_to_update

    def find_hash_and_index(self, needed_name, list_of_lines, is_directory=False):
        for i in range(len(list_of_lines)):
            current_name, hash = list_of_lines[i].split()
            if (current_name[-1] == '/' and is_directory) or (current_name[-1] == ':' and not is_directory):
                current_name = current_name[:len(current_name) - 1:]
                if current_name == needed_name:
                    return hash, i
        return None, len(list_of_lines)

    def read_file_by_hash(self, hash):
        with open(hash, 'rb') as file:
            return file.read().decode()


    def execute_command(self):
        self.parsed_path_from_input = os.path.splitext(self.path_to_file_from_input)
        self.name_of_file = self.parsed_path_from_input[0] + self.parsed_path_from_input[1]
        self.input_data = []
        for line in sys.stdin:
            self.input_data.append(line)
        directories_to_update, indexes_to_update = self.get_directories_and_indexes_to_update(self.hash_value)
        self.answer_hash = self.update_directories(directories_to_update, indexes_to_update)

    def print_answer(self):
        print(self.answer_hash)

put = CommandPut()
put.execute_command()
put.print_answer()
