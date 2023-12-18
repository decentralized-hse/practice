import queue
import threading

from abstractions import *
from utilities import split_ignore_quotes
from router import Router


class Shell:
    def __init__(self, router: Router, shell_invite: str):
        self.router = router
        self.shell_invite = shell_invite
        self.thread: threading.Thread = None
        self.output_queue = queue.Queue()

    def accept_message(self, message: str):
        self.output_queue.put("Пришло сообщение: " + message)

    def print(self, message: str):
        self.output_queue.put(message)

    def exception(self, message: str):
        self.output_queue.put("Ошибка: " + message)

    def writer(self):
        while True:
            output_data = self.output_queue.get()
            print(output_data)
            self.reset_shell()

    def reset_shell(self):
        print(self.shell_invite, end='')

    def start_shell(self):
        thread = threading.Thread(target=self.wait_for_command)
        writter = threading.Thread(target=self.writer)
        thread.start()
        writter.start()
        self.thread = thread

    def exit_shell(self):
        self.thread.join()

    def wait_for_command(self):
        while True:
            inpt = input()
            command = split_ignore_quotes(inpt)

            if len(command) == 0:
                continue
            if command[0] == 'send':
                try:
                    self.router.send_message(command[2].strip("'\"").encode(),
                                             command[1])
                except BaseException as e:
                    self.exception(str(e))
                continue
            if command[0] == 'an':
                try:
                    self.router.announce()
                except BaseException as e:
                    self.exception(str(e))
                continue
            if command[0] == 'table':
                self.print(str(self.router.table))
                continue
            if command[0] == 'friends':
                self.print(str(self.router.contacts))

            if command[0] == 'new':
                self.router.entrypoints.append(command[1])
                self.router.announce()
            if command[0] == 'af':
                self.router.contacts.append(command[1])


class ShellMessageOutput(BaseMessageOutput):
    def __init__(self):
        super().__init__()
        self.lmb = None

    def subscribe(self, lmb):
        self.lmb = lmb

    def accept_message(self, byte: bytes):
        try:
            self.lmb(byte.decode())
        except:
            self.lmb("Raw bytes: " + byte.hex())
