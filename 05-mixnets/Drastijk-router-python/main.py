import argparse
import hashlib
import random

import pysodium

from shell import *
from socket_io import *

if __name__ == "__main__":
    # Создаем парсер аргументов командной строки
    parser = argparse.ArgumentParser(description='Один из вариантов ноды для mixnets')

    # Добавляем аргументы с помощью ключей
    parser.add_argument('--node', action='append', metavar='IP', help='Добавить один из IP адресов известных вам нод')
    parser.add_argument('--own-ip', metavar='IP',
                        help='На каком IP адресе открыть сервер? 0.0.0.0 для всех подключений')
    parser.add_argument('--name', help='Ваш ключ. Это может быть никнейм или последовательность символов')

    # Разбираем аргументы командной строки
    args = parser.parse_args()

    # Получаем значения аргументов
    ip_addresses = args.node if args.node else []
    own_ip = args.own_ip
    eB = pysodium.randombytes(pysodium.crypto_scalarmult_curve25519_BYTES)
    name = pysodium.crypto_scalarmult_curve25519_base(eB)
    print("Ваш публичный ключ: " + name.hex())

    io = OurSocketIO(own_ip)
    shell_out = ShellMessageOutput()
    router = Router(ip_addresses, {}, name, io, shell_out)
    shell = Shell(router, "-> ")
    shell_out.subscribe(shell.accept_message)
    shell.start_shell()
