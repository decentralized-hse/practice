class BaseIO:
    def __init__(self):
        self.on_message = None

    def send_message(self, bytes: [bytes], address: str):
        pass

    def _on_message(self, message: [bytes], sender: str):
        self.on_message(message, sender)

    def subscribe(self, event):
        self.on_message = event


class BaseAddressResolver:
    def get_address_hash(self, contact: str):
        raise


class BaseMessageSender:
    def send_message(self, type: str, payload: [bytes], receiver: str):
        raise
