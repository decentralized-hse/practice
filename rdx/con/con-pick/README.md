`con-pick` chooses the best available state (the longest branch),

Пример запуска:

./con_pick --db ../../db

Пример входных данных: 

block1.brik
{
  "type": "block",
  "prevBlock": "",
  "txs": [],
  "nonce": "lmns54nguoq2dfg",
  "miner": "Scrooge",
  "reward": 1,
  "time": 1680000123,
  "difficultyTarget": "0000ffff",
  "balancesDelta": {
     "Alice": -50,
     "Bob": 50,
     "Scrooge": 1
  }
}

Пример выходных данных (используем имя файла как его hash для простоты): 

The best block hash is: block1




