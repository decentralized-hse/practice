import sys
from router import Router
from socket_io import *
from shell import *
import argparse

if __name__ == "__main__":
    # Создаем парсер аргументов командной строки
    parser = argparse.ArgumentParser(description='Один из вариантов ноды для mixnets')

    # Добавляем аргументы с помощью ключей
    parser.add_argument('--node', action='append', metavar='IP', help='Добавить один из IP адресов известных вам нод')
    parser.add_argument('--own-ip', metavar='IP', help='На каком IP адресе открыть сервер? 0.0.0.0 для всех подключений')
    parser.add_argument('--contact', action='append', help='Добавить один из публичных ключей известных вам контактов')
    parser.add_argument('--name', help='Ваш ключ. Это может быть никнейм или последовательность символов')

    # Разбираем аргументы командной строки
    args = parser.parse_args()

    # Получаем значения аргументов
    ip_addresses = args.node if args.node else []
    own_ip = args.own_ip
    contacts = args.contact if args.contact else []
    name = args.name

    io = OurSocketIO(own_ip)
    shell_out = ShellMessageOutput()
    router = Router(ip_addresses, contacts, name, io, shell_out)
    shell = Shell(router, "-> ")
    shell_out.subscribe(shell.accept_message)
    shell.start_shell()
