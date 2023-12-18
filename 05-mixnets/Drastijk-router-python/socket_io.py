import copy
import socket
import threading
from queue import Queue

from abstractions import BaseIO


class OurSocketIO(BaseIO):
    def __init__(self, host):
        super().__init__()
        self.queues: {str: Queue} = {}
        self.port = 8000
        self.host = host
        self.connections = {}

        thread = threading.Thread(
            target=lambda: self.start_server(host, self.port))
        thread.start()

        client_thread = threading.Thread(target=self.client_handler)
        client_thread.start()

    def send_message(self, message: bytes, address: str):
        if address not in self.queues:
            new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            new_socket.connect((address, 8000))
            new_socket.send(message)
            self.queues[address] = Queue()
            new_socket.setblocking(False)
            self.connections[address] = new_socket

        self.queues[address].put(message)

    def client_handler(self):
        while True:
            conns = copy.copy(self.connections)
            for address, conn in conns.items():
                data = []
                while True:
                    # receive data stream. it won't accept data packet greater than 1024 bytes
                    try:
                        curdata = conn.recv(64)
                        data += curdata
                    except OSError or TypeError as e:
                        if self.on_message and len(data) > 0:
                            self.on_message(bytes(data), address)
                            break


                while not self.queues[address].empty():
                    print("sending to {}".format(address))
                    conn.send(self.queues[address].get())

    def accept_connections(self, serverSocket):
        client, address = serverSocket.accept()
        client.setblocking(False)
        print('Connected to: ' + address[0] + ':' + str(address[1]))
        self.queues[address[0]] = Queue()
        self.connections[address[0]] = client

    def start_server(self, host, port):
        ServerSocket = socket.socket()
        try:
            ServerSocket.bind((host, port))
        except socket.error as e:
            print(str(e))
        print(f'Server is listing on the port {port}...')
        ServerSocket.listen()

        while True:
            self.accept_connections(ServerSocket)

    def subscribe(self, event):
        self.on_message = event

    def __exit__(self, exc_type, exc_value, traceback):
        for connection in self.connections:
            del connection
