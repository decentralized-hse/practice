import socket
import threading
from queue import Queue

from abstractions import BaseIO


class OurSocketIO(BaseIO):
    def __init__(self, host):
        super().__init__()
        self.queues: {str:Queue} = {}
        self.port = 8000
        self.host = host

        thread = threading.Thread(
            target=lambda: self.start_server(host, self.port))
        thread.start()

    def send_message(self, message: bytes, address: str):
        if address not in self.queues:
            new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            new_socket.connect((address, 8000))
            new_socket.send(message)
            self.queues[address] = Queue()
            new_thread = threading.Thread(target=self.client_handler, args=(new_socket, address), daemon=True)
            new_thread.start()
        self.queues[address].put(message)

    def client_handler(self, conn, address):
        conn.send(str.encode('You are now connected to the replay server... Type BYE to stop'))
        data = []
        while True:
            # receive data stream. it won't accept data packet greater than 1024 bytes
            curdata = conn.recv(64)
            data += curdata
            if len(curdata) < 64:
                if self.on_message:
                    self.on_message(bytes(data), address)
                while not self.queues[address].empty():
                    conn.send(self.queues[address].popleft())

    def accept_connections(self, serverSocket):
        client, address = serverSocket.accept()
        print('Connected to: ' + address[0] + ':' + str(address[1]))
        self.queues[address] = Queue()
        new_thread = threading.Thread(target=self.client_handler, args=(client,address[0]), daemon=True)
        new_thread.start()

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
