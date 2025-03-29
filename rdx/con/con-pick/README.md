`con-pick` chooses the best available state (the longest branch),

Для работы программы нужно установить библиотеку nlohmann-json:
brew install nlohmann-json

Для сборки:
cd con-pick
mkdir build
cd build
cmake ..
make 

После пересборки нужно сделать rm -rf * из папки build

--
Пример запуска:

./con_pick --db ../../db

--
Пример входных данных: 
{
  "type": "block",
  "hash": "block3_hash",
  "prevBlock": "block2",
  "txs": [],
  "nonce": "abc123xyz",
  "miner": "Bob",
  "reward": 1,
  "time": 1680000323,
  "difficultyTarget": "000000ff",
  "balancesDelta": {
     "Alice": -150,
     "Bob": 150,
     "Scrooge": 1
  }
}


Пример выходных данных: 

block1_hash




