import ed25519
import sys


def main(file_name: str) -> None:
    try:
        file = open(f"{file_name}.root", "r")
    except Exception as ex:
        print(f"[Error] Ошибка: {ex}")
        print(f"[Error] Убедитесь, что ввели имя файла без .root")
        return
    data = file.read().encode()
    print(f"[Info] файл {file_name}.root успешно прочитан.")

    private_key, public_key = ed25519.create_keypair()
    file_pub = open(f"{file_name}.pub", "wb")
    file_pub.write(public_key.to_ascii(encoding='hex'))

    file_priv = open(f"{file_name}.sec", "wb")
    file_priv.write(private_key.to_ascii(encoding='hex'))
    print(f"[Info] Ключи успешно записаны в файлы test.pub, test.sec")

    sign = private_key.sign(data, encoding='hex')
    file_sign = open(f"{file_name}.sign", "w")
    file_sign.write(sign.hex())
    print(f"[Info] Подпись успешно записана в файл test.sign")
    try:
        public_key.verify(sign, data, encoding='hex')
        print("The signature is valid.")
    except:
        print("Invalid signature!")


if __name__ == '__main__':
    try:
        file_name = sys.argv[1]
    except Exception as ex:
        print(f"[Error] Убедитесь, что ввели имя файла без .root")
        exit()
    main(file_name)
