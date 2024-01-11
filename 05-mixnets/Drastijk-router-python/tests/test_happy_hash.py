from utilities import find_happy_announce_cache_nonce, is_hash_happy, Utilities
import secrets
import pytest

def get_random_bytes(count: int = 1) -> bytes:
    return secrets.token_bytes(count)


class TestHappyHashFinding:
    @pytest.mark.parametrize(
        "hash_to_test,is_happy",
        [
            (int('0b00001111', 2).to_bytes() + get_random_bytes(20), True),
            (int('0b00000000', 2).to_bytes() + get_random_bytes(20), True),
            (int('0b00011111', 2).to_bytes() + get_random_bytes(20), False),
            (int('0b11111111', 2).to_bytes() + get_random_bytes(20), False)
        ],
    )
    def test_is_hash_happy(self, hash_to_test: bytes, is_happy: bool):
        assert is_hash_happy(hash_to_test) == is_happy

    @pytest.mark.parametrize(
        "diam", [2, 4, 6]
    )
    def test_find_happy_announce(self, diam: int):
        hash_ = get_random_bytes(8)
        happy_nonce = find_happy_announce_cache_nonce(hash_, diam)
        for i in range(diam):
            assert is_hash_happy(Utilities.sha256(hash_ + happy_nonce))
            assert is_hash_happy(hash_, happy_nonce)
            hash_ = Utilities.sha256(hash_)


