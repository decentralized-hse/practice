import copy
import select
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
        self.connections: {str: socket.socket} = {}
        self.addresses: {socket.socket: str} = {}
        self.inputs: [socket.socket] = []
        self.outputs: [socket.socket] = []

        thread = threading.Thread(
            target=lambda: self.new_client())
        thread.start()

        # client_thread = threading.Thread(target=self.client_handler)
        # client_thread.start()

    def send_message(self, message: bytes, address: str, mtu: int = None):
        if address not in self.queues:
            new_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            new_socket.connect((address, 8000))
            new_socket.send(message)
            self.queues[address] = Queue()
            new_socket.setblocking(False)
            new_socket.settimeout(1)
            self.addresses[new_socket] = address
            self.inputs.append(new_socket)

        self.queues[address].put(message)

    def new_client(self):
        sock = socket.socket()
        sock.bind(('0.0.0.0', 8000))
        sock.listen(5)
        sock.setblocking(False)
        self.inputs.append(sock)  # сокеты, которые будем читать
        # сокеты, в которые надо писать
        # здесь будем хранить сообщения для сокетов

        print('\nОжидание подключения...')
        while True:
            # вызов `select.select` который проверяет сокеты в
            # списках: `inputs`, `outputs` и по готовности, хотя бы
            # одного - возвращает списки: `reads`, `send`, `excepts`
            for potentially_input in self.inputs:
                if (potentially_input in self.addresses and
                        not self.queues[self.addresses[potentially_input]].empty()):
                    self.outputs.append(potentially_input)
            reads, send, excepts = select.select(self.inputs, self.outputs, self.inputs, 1)

            # Далее проверяются эти списки, и принимаются
            # решения в зависимости от назначения списка

            # список READS - сокеты, готовые к чтению
            for conn in reads:
                if conn == sock:
                    # если это серверный сокет, то пришел новый
                    # клиент, принимаем подключение
                    new_conn, client_addr = conn.accept()
                    print('Успешное подключение!')
                    # устанавливаем неблокирующий сокет
                    new_conn.setblocking(False)
                    # поместим новый сокет в очередь
                    # на прослушивание
                    self.inputs.append(new_conn)
                    self.outputs.append(new_conn)
                    self.addresses[new_conn] = client_addr[0]
                    self.queues[client_addr[0]] = Queue()

                else:
                    # если это НЕ серверный сокет, то
                    # клиент хочет что-то сказать
                    data = conn.recv(1024)
                    if data:
                        # если сокет прочитался и есть сообщение
                        # то кладем сообщение в словарь, где
                        # ключом будет сокет клиента
                        if self.on_message and len(data) > 0:
                            self.on_message(bytes(data), self.addresses[conn])

                        # добавляем соединение клиента в очередь
                        # на готовность к приему сообщений от сервера
                        if conn not in self.outputs:
                            self.outputs.append(conn)
                    else:
                        print('Клиент отключился...')
                        # если сообщений нет, то клиент
                        # закрыл соединение или отвалился
                        # удаляем его сокет из всех очередей
                        if conn in self.outputs:
                            self.outputs.remove(conn)
                        self.inputs.remove(conn)
                        # закрываем сокет как положено, тем
                        # самым очищаем используемые ресурсы
                        conn.close()
                        # удаляем сообщения для данного сокета

            # список SEND - сокеты, готовые принять сообщение
            for conn in send:
                # выбираем из словаря сообщения
                # для данного сокета
                queue = self.queues[self.addresses[conn]]
                if not queue.empty():
                    # если есть сообщения - то переводим
                    # его в верхний регистр и отсылаем
                    temp = queue.get()
                    print(f"sending to {self.addresses[conn]}")
                    conn.send(temp)
                else:
                    # если нет сообщений - удаляем из очереди
                    # сокетов, готовых принять сообщение
                    self.outputs.remove(conn)

            # список EXCEPTS - сокеты, в которых произошла ошибка
            for conn in excepts:
                print('Клиент отвалился...')
                # удаляем сокет с ошибкой из всех очередей
                self.inputs.remove(conn)
                if conn in self.outputs:
                    self.outputs.remove(conn)
                # закрываем сокет как положено, тем
                # самым очищаем используемые ресурсы
                conn.close()
                # удаляем сообщения для данного сокета

    def client_handler(self):
        while True:
            conns = copy.copy(self.connections)
            for address, conn in conns.items():
                print(conn.fileno())
                data = []
                while True:
                    # receive data stream. it won't accept data packet greater than 1024 bytes
                    try:
                        curdata = conn.recv(64)
                        data += curdata
                    except OSError or TypeError as e:
                        print(e)

                        break

                while not self.queues[address].empty():
                    print("sending to {}".format(address))
                    conn.send(self.queues[address].get())

    def accept_connections(self, serverSocket):
        client, address = serverSocket.accept()
        client.setblocking(False)
        client.settimeout(1)
        print('Connected to: ' + address[0] + ':' + str(address[1]))
        self.queues[address[0]] = Queue()
        self.connections[address[0]] = client

    def start_server(self, host, port):
        ServerSocket = socket.socket()
        try:
            ServerSocket.bind((host, port))
            print(f'Server is listing on the port {port}...')
        except socket.error as e:
            print(str(e))
            raise
        ServerSocket.listen()

        while True:
            self.accept_connections(ServerSocket)

    def subscribe(self, event):
        self.on_message = event

    def __exit__(self, exc_type, exc_value, traceback):
        for connection in self.connections:
            del connection
