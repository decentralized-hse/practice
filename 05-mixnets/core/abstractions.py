from models import Message


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


class BaseRouter:
    def __init__(self):
        pass

    def _schedule_next_announce(self):
        pass

    def announce(self):
        pass

    def resend_announce(self, sender: str, message: Message):
        pass

    def receive_message(self, msg: [bytes], sender: str):
        pass

    def send_message(self, msg: bytes, sender_public_key: str):
        pass

    def find_announce_match(self, target_hash, address):
        pass


class BaseMessageInput:
    def __init__(self, router: BaseRouter):
        self.router = router

    def send_message(self, msg: bytes, public_key: str):
        self.router.send_message(msg, public_key)


class BaseMessageOutput:
    def __init__(self):
        pass

    def accept_message(self, bytes):
        pass
