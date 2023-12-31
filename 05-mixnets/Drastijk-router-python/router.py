import sched
import struct
import time

from abstractions import BaseRouter, BaseIO, BaseMessageOutput
from utilities import *


class Router(BaseRouter):
    def __init__(self, entrypoints: [str],
                 contacts: {str: bytes},
                 name: bytes,
                 io: BaseIO,
                 message_output: BaseMessageOutput):
        super().__init__()
        self.message_output = message_output
        self.diam = 1000
        self.io = io
        self.io.subscribe(self.receive_message)
        self.name = name
        self.contacts = contacts
        self.entrypoints = entrypoints
        self.table = {}
        self.lastHour = 123
        self.scheduler = sched.scheduler(time.time, time.sleep)
        self.key: str = None
        self.congestion_counter = 0

    @staticmethod
    def _current_timestamp_in_bytes():
        return Utilities.get_hour_start_ns(int(time.time_ns())).to_bytes(length=8, byteorder='little')

    @staticmethod
    def _current_timestamp():
        return Utilities.get_hour_start_ns(int(time.time_ns()))

    def _schedule_next_announce(self):
        now = time.time_ns()
        next_hour = Utilities.get_hour_start_ns(int(now) + 60 * 60 * 1000000000)
        if self.lastHour == next_hour:
            return
        self.lastHour = int(next_hour)
        wait_seconds = next_hour - now
        self.scheduler.enter(int(wait_seconds / 1000000000), 1, action=self.announce)
        self.scheduler.run(blocking=False)
        print("sheduled")

    def announce(self):
        key = self.hourly_hash(self.name, self._current_timestamp())
        key_hash = Utilities.sha256(key)
        announce = serialize(Message("a", b"", key_hash))

        for point in self.entrypoints:
            self.io.send_message(announce, point)

        self._schedule_next_announce()

    def resend_announce(self, sender: str, message: Message):
        message.receiver = Utilities.sha256(message.receiver)
        for point in self.entrypoints:
            if point != sender:
                self.io.send_message(serialize(message), point)

    def receive_message(self, msg: [bytes], sender: str):
        if sender not in self.entrypoints:
            self.entrypoints.append(sender)
        try:
            message = deserialize(msg)
        except:
            print(msg)
            return
        if message.message_type == 'a':
            should_resend = self.find_announce_match(message.receiver, sender)
            if should_resend:
                self.resend_announce(sender, message)
        if message.message_type == 'M' or message.message_type == 'm':
            key = self.hourly_hash(self.name, self._current_timestamp())
            key_hash = Utilities.sha256(key)

            if key_hash == message.receiver:
                self.message_output.accept_message(message.payload)
                return

            to = message.receiver
            for key_hash, address in self.table.items():
                if Utilities.sha256(key_hash) != to:
                    continue
                message.receiver = key_hash
                print(f"{self.name} пересылаю сообщение в {address}")

                mtu = self.calculate_mtu()
                self.io.send_message(serialize(message), address, mtu=mtu)

    def calculate_mtu(self):
        self.congestion_counter += 1
        return max(1000 - self.congestion_counter * 100, 100)

    def hourly_hash(self, key, current_hour):
        print(current_hour)
        current_hour -= current_hour % (60 * 60 * 1000000000)  # округляем до часа
        timestr = struct.pack('<Q', current_hour)
        data = key + timestr[:8]
        return hashlib.sha256(data).digest()

    def send_message(self, msg: bytes, sender_public_key: str, mtu: int = None):
        print("sending message")
        if sender_public_key not in self.contacts:
            raise Exception("Неизвестный контакт")

        key = self.hourly_hash(self.contacts[sender_public_key], self._current_timestamp())
        key_hash = Utilities.sha256(key)
        print(key_hash.hex())

        for i in range(self.diam):
            if key_hash in self.table.keys():
                message = Message("M", msg, key_hash, mtu=mtu)
                self.io.send_message(serialize(message), self.table[key_hash])
                return
            key_hash = Utilities.sha256(key_hash)
        raise Exception("Маршрут до получателя не найден в таблице")

    def find_announce_match(self, target_hash, address):
        for existing_key in self.table.keys():
            next_iter = target_hash
            for i in range(self.diam):
                # проверяем если новый ключ короче ключа из таблицы
                next_iter = Utilities.sha256(next_iter)
                if next_iter == existing_key:
                    del self.table[existing_key]
                    self.table[target_hash] = address
                    return True
            next_iter = existing_key
            for i in range(self.diam):
                # проверяем если новый ключ длиннее ключа из таблицы
                next_iter = Utilities.sha256(next_iter)
                if next_iter == target_hash:
                    return False
        # новый ключ действительно новый
        self.table[target_hash] = address
        return True
