class Message:
    def __init__(self, message_type: str, payload: [bytes], receiver: bytes, iv: bytes = bytes(16)):
        assert len(message_type) == 1
        self.message_type = message_type
        self.payload = payload
        self.receiver = receiver
        self.iv = iv
