from abstractions import BaseRouter, BaseIO, BaseMessageOutput
from threading import Timer
from utilities import *
from datetime import datetime, timezone, timedelta
from models import Message
from acknowledgement_journal import AcknowledgementJournal
import threading
import time


class Router(BaseRouter):
    def __init__(self, entrypoints: [str], contacts: [str], name: str, io: BaseIO, message_output: BaseMessageOutput):
        super().__init__()
        self.message_output = message_output
        self.diam = 1000
        self.io = io
        self.io.subscribe(self.receive_message)
        self.name = name
        self.contacts = contacts
        self.entrypoints = entrypoints
        self.table = {}
        self.journal = AcknowledgementJournal()

    def _schedule_next_announce(self):
        now = datetime.now(timezone.utc)
        next_hour = Utilities.get_closes_timestamp() + timedelta(hours=1)

        wait_seconds = (next_hour - now).seconds

        threading.Timer(wait_seconds, lambda: self.announce()).start()

    def announce(self):
        closes_timestamp = Utilities.get_closes_timestamp()

        key = (self.name + str(closes_timestamp)).encode()
        announce = serialize(Message("a", b"", Utilities.sha256(key)))

        for point in self.entrypoints:
            self.io.send_message(announce, point)

        self._schedule_next_announce()

    def resend_announce(self, sender: str, message: Message):
        message.receiver = Utilities.sha256(message.receiver)
        for point in self.entrypoints:
            if point != sender:
                self.io.send_message(serialize(message), point)

    def send_ack(self, receiver, record):
        ack = Message('C', '', receiver)
        ack.ack_number = find_ack_number(record.ackMessagesIndexes)
        record.timer.cancel()
        self.io.send_message(serialize(ack), receiver)
        record.ackMessagesIndexes = list()

    def receive_message(self, msg: [bytes], sender: str):
        message = deserialize(msg)
        if message.message_type == 'a':
            should_resend = self.find_announce_match(message.receiver, sender)
            if should_resend:
                self.resend_announce(sender, message)
        if message.message_type == 'M' or message.message_type == 'm' or \
                message.message_type == 'C' or message.message_type == 'c':
            timestamp = str(Utilities.get_closes_timestamp())
            key_hash = (self.name + timestamp).encode()
            me_hash = Utilities.sha256(key_hash)
            if me_hash == message.receiver:
                self.message_output.accept_message(message.payload)
                if message.message_type == 'M' or message.message_type == 'm':
                    return
            to = message.receiver
            for key_hash, address in self.table.items():
                if Utilities.sha256(key_hash) != to:
                    continue
                message.receiver = key_hash
                print(f"{self.name} пересылаю сообщение в {address}")
                self.io.send_message(serialize(message), address)
        # TODO: буфер добавить
        if message.message_type == 'C' or message.message_type == 'c':
            if message.ack_number != -1:
                record = self.journal.sent_messages[message.session_id]
                record.lastAckMsg = message.ack_number
                return
            if not self.journal.received_messages[message.session_id]:
                self.journal.add_new_recv_session(message.window_size, message.session_id)
            record = self.journal.received_messages[message.session_id]
            if len(record) == 0:
                record.timer = Timer(10, self.send_ack, args=(message.sender, record))
                record.timer.start()
            record.ackMessagesIndexes.append(message.message_number)
            if len(record.ackMessagesIndexes) == message.window_size:
                self.send_ack(message.sender, record)

    def send_message(self, msg: bytes, sender_public_key: str):
        timestamp = str(Utilities.get_closes_timestamp())
        key_hash = (sender_public_key + timestamp).encode()
        for i in range(self.diam):
            key_hash = Utilities.sha256(key_hash)
            if key_hash in self.table.keys():
                message = Message("M", msg, key_hash)
                self.io.send_message(serialize(message), self.table[key_hash])
                return
        raise Exception("Маршрут до получателя не найден в таблице")

    def send_message_with_ack(self, msg: bytes, sender_public_key: str):
        session_id = self.journal.add_new_session(msg, 3, 10)
        while not self.journal.is_session_finished(session_id):
            timestamp = str(Utilities.get_closes_timestamp())
            key_hash = (sender_public_key + timestamp).encode()
            my_key_hash = (self.name + timestamp).encode()
            msg_parts = self.journal.get_next_messages(session_id)
            for i in range(self.diam):
                key_hash = Utilities.sha256(key_hash)
                my_key_hash = Utilities.sha256(my_key_hash)
                if key_hash in self.table.keys():
                    prepared_parts = []
                    for part_with_index in msg_parts:
                        prepared_part = Message("C", part_with_index[0], key_hash)
                        prepared_part.add_delivery_ack(
                            part_with_index[1],
                            -1,
                            session_id,
                            session_id.hex,
                            len(msg_parts),
                            my_key_hash)
                        prepared_parts.append(prepared_part)
                    for part in prepared_parts:
                        self.io.send_message(serialize(part), self.table[key_hash])
                    break
            time.sleep(0.001)

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
