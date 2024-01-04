from abstractions import BaseAcknowledgementJournal
from journal_record import JournalSentRecord, JournalRecvRecord
import uuid


class AcknowledgementJournal(BaseAcknowledgementJournal):
    def __init__(self):
        super().__init__()
        self.received_messages = {}
        self.sent_messages = {}

    def add_new_recv_session(self, window, session_id):
        new_record = JournalRecvRecord(window)
        self.received_messages[session_id] = new_record

    def add_new_session(self, full_message, window, message_size):
        new_record = JournalSentRecord(window, full_message, message_size)
        new_uuid = uuid.uuid4()
        self.sent_messages[new_uuid] = new_record
        return new_uuid

    def is_session_finished(self, session_id):
        if session_id not in self.sent_messages:
            raise Exception("Session not exist")
        record = self.sent_messages[session_id]
        return record.lastAckMsg == len(record.part_with_index)

    def get_next_messages(self, session_id):
        if session_id not in self.sent_messages:
            raise Exception("Session not exist")
        record = self.sent_messages[session_id]
        lastAckMsg = record.lastAckMsg
        if lastAckMsg < record.sentMessageCount:
            return record.part_with_index[lastAckMsg + 1:record.sentMessageCount + 1]

        record.sentMessageCount += record.window
        return record.part_with_index[lastAckMsg + 1:record.sentMessageCount + 1]
