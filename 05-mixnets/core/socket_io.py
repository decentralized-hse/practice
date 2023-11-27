import socket
import threading

from abstractions import BaseIO


class OurSocketIO(BaseIO):
    def __init__(self, host):
        super().__init__()
        self.connections = {}
        self.port = 8008
        self.host = host

        thread = threading.Thread(target=lambda: self.start_server(host, self.port))
        thread.start()

    def send_message(self, message: bytes, address: str):
        if address not in self.connections:
            try:
                new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                new_socket.connect((address, 8008))
                self.connections[address] = new_socket
            except:
                new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                new_socket.connect((address, 8008))
                self.connections[address] = new_socket

        try:
            current_connection = self.connections[address]
            current_connection.send(message)
        except:
            new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            new_socket.connect((address, 8008))
            self.connections[address] = new_socket

    def start_server(self, host, port):
        server_socket = socket.socket()  # get instance
        # look closely. The bind() function takes tuple as argument
        server_socket.bind((host, port))  # bind host address and port together

        # configure how many client the server can listen simultaneously
        server_socket.listen(2)
        while True:
            conn, address = server_socket.accept()  # accept new connection
            data = []
            while True:
                # receive data stream. it won't accept data packet greater than 1024 bytes
                curdata = conn.recv(1024)
                data += curdata
                if len(curdata) < 1024:
                    # if data is not received break
                    break

            if self.on_message:
                self.on_message(bytes(data), address[0])

            conn.close()  # close the connection

    def subscribe(self, event):
        self.on_message = event

    def __exit__(self, exc_type, exc_value, traceback):
        for connection in self.connections:
            del connection
