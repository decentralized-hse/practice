import struct
from datetime import datetime, timezone
import hashlib
import re
from models import Message


class Utilities:
    @staticmethod
    def hour_rounder_floor(t: datetime) -> datetime:
        return t.replace(second=0, microsecond=0, minute=0, hour=t.hour)

    @staticmethod
    def get_closes_timestamp() -> datetime:
        now = datetime.now(timezone.utc)
        closes_timestamp = Utilities.hour_rounder_floor(now)
        return closes_timestamp

    @staticmethod
    def get_hour_start_ns(time_ns: int) -> int:
        time_ns -= time_ns % (60 * 60 * 1000000000)
        return time_ns

    @staticmethod
    def sha256(data) -> bytes:
        return hashlib.sha256(data).digest()


CACHE_HAPPINESS_MASK = int('0b11100000', 2)  # маска для первых 3 бит
ATTEMPTS_TO_FIND_HAPPY_CACHE_NONCE = 10**7


def is_hash_happy(hash_to_check: bytes) -> bool:
    """
    Метод, проверяющий, что первые 3 бита кэша - нули.

    Параметры:
    nonce - если передан, на счастливость проверяется sha256 от announce_hash + nonce, иначе просто announce_hash
    """
    return hash_to_check[0] & CACHE_HAPPINESS_MASK == 0


def find_happy_announce(key: bytes, timestamp: int, diam: int) -> bytes:
    while True:
        key_hash = Utilities.sha256(key + struct.pack('n', timestamp))
        happy_hash = key_hash
        for i in range(diam):
            key_hash = Utilities.sha256(key_hash)
            if not is_hash_happy(key_hash):
                break
        else:
            return happy_hash

        timestamp += 1


def serialize(message: Message):
    message_length_length = 4 if message.message_type.capitalize() == message.message_type else 1
    message_bytes = (
            message.message_type.encode() +
            (len(message.payload) + 32).to_bytes(message_length_length, "little") +
            message.receiver +
            message.payload)

    return message_bytes


def deserialize(message: bytes):
    type_ = message[0].to_bytes(1, "little").decode()
    message_length_length = 4 if type_.capitalize() == type_ else 1
    message_length = int.from_bytes(message[1:(1 + message_length_length)])
    address = message[message_length_length + 1:message_length_length + 32 + 1]
    payload = message[1 + message_length_length + 32:
                      1 + message_length_length + 32 + message_length]
    return Message(type_, payload, address)


def split_ignore_quotes(string):
    # Разделение строки по пробелам, но не разделять то, что в кавычках
    pattern = r'"[^"]*"|\S+'  # Паттерн для поиска подстрок в кавычках или непосредственно последовательностей непробельных символов
    substrings = re.findall(pattern, string)
    return substrings
