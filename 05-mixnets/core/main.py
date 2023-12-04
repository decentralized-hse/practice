import sys
from router import Router
from socket_io import *
from shell import *


if __name__ == "__main__":
    address = sys.argv[1]
    contacts = sys.argv[2].split(",")
    entrypoints = sys.argv[3].split(",")
    name = sys.argv[4]
    ip = address
    io = OurSocketIO(ip)
    shell_out = ShellMessageOutput()
    router = Router(entrypoints, contacts, name, io, shell_out)
    shell = Shell(router, "-> ")
    shell_out.subscribe(shell.accept_message)
    shell.start_shell()
