import datetime
import hashlib


class Utilities:
    @staticmethod
    def hour_rounder(t):
        return t.replace(second=0, microsecond=0, minute=0, hour=t.hour)

    @staticmethod
    def get_closes_timestamp():
        now = datetime.datetime.utcnow()
        closes_timestamp = Utilities.hour_rounder(now)
        return closes_timestamp

    @staticmethod
    def sha256(data) -> bytes:
        return hashlib.sha256(data).digest()

