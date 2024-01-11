from shell import *
from router import *

class PrintLineMessageOutput(BaseMessageOutput):
    def __init__(self, name: str):
        super().__init__()
        self.name = name

    def accept_message(self, message: bytes):
        print(f"{self.name} получил сообщение с текстом: {message.decode()}")

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

if __name__ == "__main__":
    TestEnvironment().script()