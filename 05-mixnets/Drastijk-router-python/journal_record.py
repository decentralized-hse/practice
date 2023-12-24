class JournalSentRecord:
    def __init__(self, window, full_message, message_size):
        self.sentMessageCount = -1
        self.ackMessageCount = -1
        self.window = window
        message_parts = [full_message[i:i + message_size] for i in range(0, len(full_message), message_size)]
        self.part_with_index = [(message_parts[i], i) for i in range(0, len(message_parts))]
