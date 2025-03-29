import hashlib
import json
import os
import time
from typing import List, Dict

DATABASE_PATH = "/tmp/.con/db"
MEMPOOL_PATH = "/tmp/.con/mempool"
BLOCK_REWARD = 1
DIFFICULTY_TARGET = "0000"
MAX_TRANSACTION_COUNT = 10

# TODO: change json -> brix
def load_json(file_path: str) -> Dict:
    with open(file_path, 'r') as file:
        return json.load(file)

# TODO: change json -> brix
def save_json(data: Dict, file_path: str) -> None:
    with open(file_path, 'w') as file:
        json.dump(data, file, indent=4)

# TODO: should be common method for concoin
def calculate_hash(data: str) -> str:
    return hashlib.sha256(data.encode('utf-8')).hexdigest()

# TODO: change to con-valid
def validate_transaction(transaction: Dict) -> bool:
    return True

# TODO: change to con-pick
def get_best_block() -> Dict:
    files = os.listdir(DATABASE_PATH)
    best_block = None
    max_difficulty = -1

    for file in files:
        block = load_json(os.path.join(DATABASE_PATH, file))
        difficulty = len(block["difficultyTarget"])
        if difficulty > max_difficulty:
            best_block = block
            max_difficulty = difficulty

    return best_block

# TODO: change to con-run
def publish_block(block: Dict) -> None:
    block_file_path = os.path.join(DATABASE_PATH, f"{block['hash']}.json")
    save_json(block, block_file_path)

def get_mempool_transactions() -> List[Dict]:
    files = os.listdir(MEMPOOL_PATH)
    transactions = []

    for file in files:
        transaction = load_json(os.path.join(MEMPOOL_PATH, file))
        transactions.append(transaction)

    return transactions

def calculate_balances_delta(transactions: List[Dict], miner_id: str = "") -> Dict:
    delta = {}

    for transaction in transactions:
        if transaction["from"] not in delta:
            delta[transaction["from"]] = 0
        delta[transaction["from"]] -= transaction["amount"]
        if transaction["to"] not in delta:
            delta[transaction["to"]] = 0
        delta[transaction["to"]] += transaction["amount"]

    if miner_id:
        if miner_id not in delta:
            delta[miner_id] = 0
        delta[miner_id] += BLOCK_REWARD

    return delta

def mine_block(best_block: Dict, transactions: List[Dict], miner_id: str, difficulty_target: str = DIFFICULTY_TARGET, malicious: bool = False) -> Dict:
    block_data = {
        "type": "block",
        "prevBlock": best_block["hash"],
        "txs": transactions,
        "nonce": "",
        "miner": miner_id,
        "reward": BLOCK_REWARD,
        "time": int(time.time()),
        "difficultyTarget": difficulty_target,
        "balancesDelta": calculate_balances_delta(transactions, miner_id),
    }

    if malicious:
        block_data["balancesDelta"][miner_id] += BLOCK_REWARD

    nonce = 0
    while True:
        block_data["nonce"] = str(nonce)
        serialized_block = json.dumps(block_data, sort_keys=True)
        block_hash = calculate_hash(serialized_block)
        if block_hash.startswith(difficulty_target) or malicious:
            block_data["hash"] = block_hash
            break
        nonce += 1

    return block_data

def main():
    import argparse

    parser = argparse.ArgumentParser(description="ConCoin Mining Module")
    parser.add_argument("--miner-id", required=True, help="ID of the miner")
    parser.add_argument("--target", default=DIFFICULTY_TARGET, help="difficulty target for nonce")
    parser.add_argument("--transaction-count", default=MAX_TRANSACTION_COUNT, type=int, help="max transaction count")
    parser.add_argument("--malicious", action="store_true", help="malicious mode")

    args = parser.parse_args()

    best_block = get_best_block()
    if not best_block:
        print("Error: No blocks found in the database.")
        return

    transactions = get_mempool_transactions()
    valid_transactions = [tx for tx in transactions if validate_transaction(tx)][:args.transaction_count]
    if not valid_transactions:
        print("Error: No valid transactions found in the mempool.")
        return

    new_block = mine_block(best_block, valid_transactions, args.miner_id, args.target, args.malicious)

    publish_block(new_block)
    print(f"Successfully mined and published block with hash {new_block['hash']}.")

if __name__ == "__main__":
    main()
