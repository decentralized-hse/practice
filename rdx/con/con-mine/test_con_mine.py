import unittest
from con_mine import mine_block

class TestMineBlock(unittest.TestCase):
    def setUp(self):
        self.best_block = {
            "type": "block",
            "hash": "0000abcd1234",
            "difficultyTarget": "0000",
            "balancesDelta": {"Alice": 100},
            "txs": [],
            "nonce": "",
            "miner": "",
            "reward": 1,
            "time": 1680000123,
            "prevBlock": None
        }

        self.transactions = [
            {"type": "transaction", "from": "Alice", "to": "Bob", "amount": 50, 
             "signature": "VALID_SIGNATURE", "pubKey": "ED25519PUBKEYAlice"},
            {"type": "transaction", "from": "Charlie", "to": "Dave", "amount": 30, 
             "signature": "VALID_SIGNATURE", "pubKey": "ED25519PUBKEYCharlie"}
        ]

        self.miner_id = "Scrooge"

    def test_mine_block_produces_valid_nonce(self):
        new_block = mine_block(self.best_block, self.transactions, self.miner_id, "000")

        self.assertTrue(new_block["hash"].startswith("000"))
        self.assertNotEqual(new_block["nonce"], "")

    def test_mine_block_includes_transactions(self):
        new_block = mine_block(self.best_block, self.transactions, self.miner_id)

        self.assertEqual(len(new_block["txs"]), len(self.transactions))
        self.assertEqual(new_block["balancesDelta"]["Alice"], -50)
        self.assertEqual(new_block["balancesDelta"]["Bob"], 50)

    def test_mine_block_rewards_miner_correctly(self):
        new_block = mine_block(self.best_block, self.transactions, self.miner_id)

        self.assertEqual(new_block["balancesDelta"][self.miner_id], 1)
        self.assertEqual(new_block["reward"], 1)
        self.assertEqual(new_block["miner"], self.miner_id)

    def test_mine_block_sets_correct_prevBlock(self):
        new_block = mine_block(self.best_block, self.transactions, self.miner_id)

        self.assertEqual(new_block["prevBlock"], self.best_block["hash"])

    def test_mine_block_malicious(self):
        new_block = mine_block(self.best_block, self.transactions, self.miner_id, "00000000", True)

        self.assertTrue(not new_block["hash"].startswith("00000000"))
        self.assertNotEqual(new_block["nonce"], "")
        self.assertEqual(new_block["balancesDelta"][self.miner_id], 2)

if __name__ == '__main__':
    unittest.main()
