`con-pick` chooses the best available state (the longest branch),

Для работы программы нужно установить библиотеки nlohmann-json и googletest:
brew install nlohmann-json
brew install googletest

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

Пример запуска тестов:

ctest —V (вызываем из папки build)

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




