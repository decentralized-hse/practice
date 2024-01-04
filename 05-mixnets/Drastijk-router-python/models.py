from uuid import UUID


class Message:
    def __init__(self, message_type: str, payload: [bytes], receiver: bytes):
        self.sender, self.window_size, self.session_id, self.ack_number, self.message_number = \
            None, None, None, None, None
        assert len(message_type) == 1
        self.message_type = message_type
        self.payload = payload
        self.receiver = receiver

    def add_delivery_ack(self, message_number: int, ack_number: int, session_id: str, window_size: int, sender: bytes):
        self.message_number = message_number
        self.ack_number = ack_number
        self.session_id = session_id
        self.window_size = window_size
        self.sender = sender
