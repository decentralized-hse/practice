class MessageEnumerator:
    def __init__(self, message: bytes, start=0):
        self.left = start
        self.right = start
        self.message = message

    def get_next_bytes(self, length):
        self.left = self.right
        self.right += length
        return self.message[self.left:self.right]
