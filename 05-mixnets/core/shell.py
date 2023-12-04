from abstractions import *
from utilities import split_ignore_quotes
import threading
import queue
class Shell:
    def __init__(self, router: BaseRouter, shell_invite: str):
        self.router = router
        self.shell_invite = shell_invite
        self.thread: threading.Thread = None
        self.output_queue = queue.Queue()

    def accept_message(self, message: str):
        self.output_queue.put(message)

    def writer(self):
        while True:
            output_data = self.output_queue.get()
            print(output_data)
            if self.output_queue.empty():
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
            print(inpt)
            command = split_ignore_quotes(inpt)

            if len(command) == 0:
                continue
            if command[0] == 'send':
                try:
                    print("send")
                    self.router.send_message(command[2].strip("'\"").encode(), command[1])
                except BaseException as e:
                    self.accept_message(str(e))
                continue
            if command[0] == 'an':
                try:
                    self.router.announce()
                except BaseException as e:
                    self.accept_message(str(e))
                continue
            if command[0] == 'table':
                self.accept_message(str(self.router.table))
                continue
            if command[0] == 'friends':
                self.accept_message(str(self.router.contacts))


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