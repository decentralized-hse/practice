import hashlib
import threading

from abstractions import *


import re

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
    payload = message[message_length_length + 1 + 32:]
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

    @staticmethod
    def sha256(data) -> bytes:
        return hashlib.sha256(data).digest()

    def announce(self):
        announce = serialize(Message("a", b"", self.sha256(self.name.encode())))
        for point in self.entrypoints:
            self.io.send_message(announce, point)

    def resend_announce(self, sender: str, message: Message):
        message.receiver = self.sha256(message.receiver)
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
            me_hash = self.sha256(self.name.encode())
            if me_hash == message.receiver:
                self.message_output.accept_message(message.payload)
                return
            to = message.receiver
            for key_hash, address in self.table.items():
                if self.sha256(key_hash) != to:
                    continue
                message.receiver = key_hash
                print(f"{self.name} пересылаю сообщение в {address}")
                self.io.send_message(serialize(message), address)

    def send_message(self, msg: bytes, sender_public_key: str):
        key_hash = sender_public_key.encode()
        for i in range(self.diam):
            key_hash = self.sha256(key_hash)
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
                next_iter = self.sha256(next_iter)
                if next_iter == existing_key:
                    del self.table[existing_key]
                    self.table[target_hash] = address
                    return True
            next_iter = existing_key
            for i in range(self.diam):
                # проверяем если новый ключ длиннее ключа из таблицы
                next_iter = self.sha256(next_iter)
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

    def accept_message(self, message: str):
        print(message)
        self.reset_shell()

    def reset_shell(self):
        print(self.shell_invite, end='')

    def start_shell(self):
        thread = threading.Thread(target=self.wait_for_command)
        thread.start()
        self.thread = thread

    def exit_shell(self):
        self.thread.join()

    def wait_for_command(self):
        while True:
            self.reset_shell()
            command = split_ignore_quotes(input())

            if len(command) == 0:
                continue
            if command[0] == 'send':
                try:
                    self.router.send_message(command[2].strip("'\"").encode(), command[1])
                except BaseException as e:
                    print(e)
                    continue
            if command[0] == 'friends':
                print(self.router.contacts)


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
    TestEnvironment().script()
