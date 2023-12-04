import datetime
import hashlib
from models import Message
import re
class Utilities:
    @staticmethod
    def hour_rounder(t):
        return t.replace(second=0, microsecond=0, minute=0, hour=t.hour)

    @staticmethod
    def get_closes_timestamp():
        now = datetime.datetime.now(datetime.UTC)
        closes_timestamp = Utilities.hour_rounder(now)
        return closes_timestamp

    @staticmethod
    def sha256(data) -> bytes:
        return hashlib.sha256(data).digest()


def serialize(message: Message):
    message_length_length = 4 if message.message_type.capitalize() == message.message_type else 1
    message_bytes = (message.message_type.encode() +
                     (len(message.payload) + 32).to_bytes(message_length_length) +
                     message.receiver +
                     message.payload)

    return message_bytes


def deserialize(message: bytes):
    type = message[0].to_bytes(1).decode()
    message_length_length = 4 if type.capitalize() == type else 1
    message_length = int.from_bytes(message[1:(1 + message_length_length)])
    address = message[message_length_length + 1:message_length_length + 32 + 1]
    payload = message[1 + message_length_length + 32:1 + message_length_length + 32 + message_length]
    return Message(type, payload, address)

def split_ignore_quotes(string):
    # Разделение строки по пробелам, но не разделять то, что в кавычках
    pattern = r'"[^"]*"|\S+'  # Паттерн для поиска подстрок в кавычках или непосредственно последовательностей непробельных символов
    substrings = re.findall(pattern, string)
    return substrings