from socket_io import OurSocketIO


def main():
    ip1, port1 = "127.0.0.1", 8888
    ip2, port2 = "127.0.0.1", 9999
    sock1 = OurSocketIO(ip1, port1)
    sock1.subscribe(handler1)
    sock2 = OurSocketIO(ip2, port2)
    sock2.subscribe(handler2)

    sock1.send_message(b"11111", f"{ip2}:{port2}")

    sock2.send_message(b"11111", f"{ip1}:{port1}")


def handler1(data: bytes):
    print("handler1 " + data.hex())


def handler2(data: bytes):
    print("handler2 " + data.hex())


if __name__ == "__main__":
    main()
