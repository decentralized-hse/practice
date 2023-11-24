from main import *





def start_shell():
    public_key = input("Введите ваш публичный ключ: ")
    address = input("Введите адрес: ")
    contacts = input("Введите известные вам публичные ключи контактов через запятую: ").split(",")
    entrypoints = input("Введите известные вам ноды: ").split(",")


if __name__ == "__main__":
    start_shell()
