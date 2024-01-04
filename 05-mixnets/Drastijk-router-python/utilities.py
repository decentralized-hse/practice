from datetime import datetime, timezone
import hashlib
import re
from models import Message
from message_enumerator import MessageEnumerator


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


def serialize(message: Message):
    message_length_length = 4 if message.message_type.capitalize() == message.message_type else 1
    message_bytes = (
            message.message_type.encode() +
            (len(message.payload)).to_bytes(message_length_length, "little") +
            message.receiver +
            message.payload)

    if message.message_type == 'C' or message.message_type == 'c':
        if message.message_number is None:
            pass

        message_bytes += (
                message.message_number.to_bytes(4, "little") +
                message.ack_number.to_bytes(4, "little") +
                message.session_id.encode() +
                message.window_size.to_bytes(4, "little") +
                message.sender
        )

    return message_bytes


def deserialize(message: bytes):
    message_enumerator = MessageEnumerator(message, 1)

    type_ = message[0].to_bytes(1, "little").decode()
    message_length_length = 4 if type_.capitalize() == type_ else 1
    message_length = int.from_bytes(message_enumerator.get_next_bytes(message_length_length), "little")
    receiver_address = message_enumerator.get_next_bytes(32)
    payload = message_enumerator.get_next_bytes(message_length)

    msg = Message(type_, payload, receiver_address)

    if type_ != 'C' and type_ != 'c':
        return msg

    message_number = int.from_bytes(message_enumerator.get_next_bytes(4), "little")
    ack_number = int.from_bytes(message_enumerator.get_next_bytes(4), "little")
    session_id = message_enumerator.get_next_bytes(32).decode()
    window_size = int.from_bytes(message_enumerator.get_next_bytes(4), "little")
    sender_address = message_enumerator.get_next_bytes(32)

    msg.add_delivery_ack(message_number, ack_number, session_id, window_size, sender_address)

    return msg


def split_ignore_quotes(string):
    # Разделение строки по пробелам, но не разделять то, что в кавычках
    pattern = r'"[^"]*"|\S+'  # Паттерн для поиска подстрок в кавычках или непосредственно последовательностей непробельных символов
    substrings = re.findall(pattern, string)
    return substrings


def get_next_msg(nums: list):
    sort_nums = sorted(nums, key=lambda x: x[1])
    for i in range(1, len(nums)):
        if sort_nums[i][1] - sort_nums[i - 1][1] != 1:
            return sort_nums[i - 1][1]
    return sort_nums[-1][1]
