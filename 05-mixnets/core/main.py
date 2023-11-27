import datetime
import re
import sys
from queue import Queue

from abstractions import *
from socket_io import *
from utilities import Utilities


def split_ignore_quotes(string):
    # Разделение строки по пробелам, но не разделять то, что в кавычках
    pattern = r'"[^"]*"|\S+'  # Паттерн для поиска подстрок в кавычках или непосредственно последовательностей непробельных символов
    substrings = re.findall(pattern, string)
    return substrings


class MessageSender(BaseMessageSender):
    def __init__(self):
        pass


class Message:
    def __init__(self, message_type: str, payload: [bytes], receiver: bytes):
        assert len(message_type) == 1
        self.message_type = message_type
        self.payload = payload
        self.receiver = receiver


def serialize(message: Message):
    message_length_length = 4 if message.message_type.capitalize() == message.message_type else 1
    message_bytes = (message.message_type.encode() +
                     (len(message.payload) + 32).to_bytes(message_length_length) +
                     message.receiver +
                     message.payload)

    return message_bytes


def deserialize(message: bytes):
    type = message[0].to_bytes(1).decode()
    message_length_length = 4 if type.capitalize() == type else 1
    message_length = int.from_bytes(message[1:(1 + message_length_length)])
    address = message[message_length_length + 1:message_length_length + 32 + 1]
    payload = message[1 + message_length_length + 32:1 + message_length_length + 32 + message_length]
    return Message(type, payload, address)


class BaseMessageOutput:
    def __init__(self):
        pass

    def accept_message(self, bytes):
        pass


class PrintLineMessageOutput(BaseMessageOutput):
    def __init__(self, name: str):
        super().__init__()
        self.name = name

    def accept_message(self, message: bytes):
        print(f"{self.name} получил сообщение с текстом: {message.decode()}")


class Router:
    def __init__(self, entrypoints: [str], contacts: [str], name: str, io: BaseIO, message_output: BaseMessageOutput):
        self.message_output = message_output
        self.diam = 1000
        self.io = io
        self.io.subscribe(self.receive_message)
        self.name = name
        self.contacts = contacts
        self.entrypoints = entrypoints
        self.table = {}

    def _schedule_next_announce(self):
        now = datetime.datetime.now(datetime.UTC)
        next_hour = Utilities.get_closes_timestamp() + datetime.timedelta(hours=1)

        wait_seconds = (next_hour - now).seconds

        threading.Timer(wait_seconds, lambda: self.announce()).start()

    def announce(self):
        closes_timestamp = Utilities.get_closes_timestamp()

        key = (self.name + str(closes_timestamp)).encode()
        announce = serialize(Message("a", b"", Utilities.sha256(key)))

        for point in self.entrypoints:
            self.io.send_message(announce, point)

        self._schedule_next_announce()

    def resend_announce(self, sender: str, message: Message):
        message.receiver = Utilities.sha256(message.receiver)
        for point in self.entrypoints:
            if point != sender:
                self.io.send_message(serialize(message), point)

    def receive_message(self, msg: [bytes], sender: str):
        message = deserialize(msg)
        if message.message_type == 'a':
            should_resend = self.find_announce_match(message.receiver, sender)
            if should_resend:
                self.resend_announce(sender, message)
        if message.message_type == 'M' or message.message_type == 'm':
            timestamp = str(Utilities.get_closes_timestamp())
            key_hash = (self.name + timestamp).encode()
            me_hash = Utilities.sha256(key_hash)
            if me_hash == message.receiver:
                self.message_output.accept_message(message.payload)
                return
            to = message.receiver
            for key_hash, address in self.table.items():
                if Utilities.sha256(key_hash) != to:
                    continue
                message.receiver = key_hash
                print(f"{self.name} пересылаю сообщение в {address}")
                self.io.send_message(serialize(message), address)

    def send_message(self, msg: bytes, sender_public_key: str):
        timestamp = str(Utilities.get_closes_timestamp())
        key_hash = (sender_public_key + timestamp).encode()
        for i in range(self.diam):
            key_hash = Utilities.sha256(key_hash)
            if key_hash in self.table.keys():
                message = Message("M", msg, key_hash)
                self.io.send_message(serialize(message), self.table[key_hash])
                return
        raise

    def find_announce_match(self, target_hash, address):
        for existing_key in self.table.keys():
            next_iter = target_hash
            for i in range(self.diam):
                # проверяем если новый ключ короче ключа из таблицы
                next_iter = Utilities.sha256(next_iter)
                if next_iter == existing_key:
                    del self.table[existing_key]
                    self.table[target_hash] = address
                    return True
            next_iter = existing_key
            for i in range(self.diam):
                # проверяем если новый ключ длиннее ключа из таблицы
                next_iter = Utilities.sha256(next_iter)
                if next_iter == target_hash:
                    return False
        # новый ключ действительно новый
        self.table[target_hash] = address
        return True


class BaseMessageInput:
    def __init__(self, router: Router):
        self.router = router

    def send_message(self, msg: bytes, public_key: str):
        self.router.send_message(msg, public_key)


class TestEnvironment:
    def __init__(self):
        self.shell_out = ShellMessageOutput()
        self.outputs = {"a": PrintLineMessageOutput("a"),
                        "b": PrintLineMessageOutput("b"),
                        "c": PrintLineMessageOutput("c"),
                        "d": self.shell_out}
        self.IOs = {"a": TestIO(self, "a"),
                    "b": TestIO(self, "b"),
                    "c": TestIO(self, "c"),
                    "d": TestIO(self, "d")}
        self.nodes = {"a": Router(["b"], ["d_*****"], "a_*****", self.IOs["a"], self.outputs["a"]),
                      "b": Router(["a", "c", "d"], [], "b_*****", self.IOs["b"], self.outputs["b"]),
                      "c": Router(["b", "d"], ["a_*****"], "c_*****", self.IOs["c"], self.outputs["c"]),
                      "d": Router(["c", "b"], ["a_*****"], "d_*****", self.IOs["d"], self.outputs["d"])}
        self.shell = Shell(self.nodes["d"], "|=> ")
        self.shell_out.subscribe(self.shell.accept_message)
        self.shell.start_shell()

    def accept_message(self, msg: [bytes], sender: str, receiver: str):
        self.IOs[receiver]._on_message(msg, sender)

    def script(self):
        for addr, node in self.nodes.items():
            node.announce()

        self.nodes["a"].send_message("ты *****".encode(), "d_*****")
        self.nodes["d"].send_message("сам ты *****".encode(), "a_*****")
        self.nodes["c"].send_message("да вы все *****ы".encode(), "a_*****")


class TestIO(BaseIO):
    def __init__(self, test_env: TestEnvironment, address):
        super().__init__()
        self.address = address
        self.test_env = test_env

    def send_message(self, bytes: [bytes], address: str):
        self.test_env.accept_message(bytes, self.address, address)


class Shell:
    def __init__(self, router: Router, shell_invite: str):
        self.router = router
        self.shell_invite = shell_invite
        self.thread: threading.Thread = None
        self.output_queue = Queue()

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


if __name__ == "__main__":
    address = sys.argv[1]
    contacts = sys.argv[2].split(",")
    entrypoints = sys.argv[3].split(",")
    name = sys.argv[4]
    ip = address
    io = OurSocketIO(ip)
    shell_out = ShellMessageOutput()
    router = Router(entrypoints, contacts, name, io, shell_out)
    shell = Shell(router, "8==D ")
    shell_out.subscribe(shell.accept_message)
    shell.start_shell()
