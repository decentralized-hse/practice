#   ConCoin commands and data formats

ConCoin is a BitCoin-inspired cryptocurrency.
It uses RDX SST files for everything: database, blocks, transactions.
RDX SST is a SST file where keys are 128-bit RDX ids and values are RDX elements.
RDX SST file can be cryptographically linked to other files by their hashes.
It can also be signed.

The general scheme of ConCoin is as follows:

 1. ConCoin nodes form an overlay network using PEX, gossip new transactions and blocks,
 2. Miner nodes try to mine a lucky block on top of the best available state,
 3. Proof of work dynamics is the same as BitCoin has.

File layout is as follows:

  - `.con/db` contains the database, including all the accepted blocks,
  - `.con/mempool/blockhash` contains proposed transactions for a block,

In the database, the `cc-1` object is an RDX E element mapping user ids to coin balances.
The `cc-2` object is the nonce as used in blocks for PoW.
The `cc-3` object is an RDX E element mapping user ids to their public keys.
Each new block only contains the updated balances.
To create an account, one has to send a transaction sending money to that id AND creating its pub key entry.
User id `cc` refers to the miner (when promising a commission).
Block reward is 1 coin.
````
cc-1: {
    Scrooge: 1000000000,
},
cc-2: lmns54nguoq2dfg,
cc-3: {
    Scrooge: ED25519PUBKEYHEX
}
````

Commands:

 1. `con-pick` chooses the best available state (the longest branch),
 2. `con-send` creates, signs and sends a transaction on top of the state,
 3. `con-mine` mines a block for the transactions in the mempool of the best state,
 4. `con-valid` validates a transaction or a block against the respective state,
 5. `con-run` accepts and relays transactions and blocks, maintains db and mempool.

Every command can have a malicious mode to try stealing moneys from other participants.

Invariant: using commands 1-5 in a loop to advance the state of the system and send money.
The total amount of coins increases by 1 with each block, nothing gets stolen.
What `send` and `mine` create non-maliciously should pass `valid` checks,
anything malicious should not. A newly mined block should be picked by `pick`.
