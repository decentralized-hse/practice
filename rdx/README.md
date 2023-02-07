#   2025 command practice

>   What I can't create I don't understand // Richard Feynman

This year, we reimplement simplified versions of the systems we talk about.
Decentralized revision control, content delivery, cryptocurrency and other things
we now implement (almost) from scratch.

This repo lays out the CLI interface of the systems in question and their RDX data
models. RDX is the Replicated Data eXchange format, you may think of it as of
"JSON with diff and patch". RDX does data synchronization, which tends to be tricky,
you do the rest.

The validity is judged by running your program through:

 1. test cases (inputs/outputs),
 2. invariants defined through other programs,
 3. fuzz testing of the invariants.

You can use any programming language from the list:

 1. go
 2. Rust
 3. C (+1)
 4. C++ (+1)
 5. python (-1)
 6. bash

Note that fuzzing can be tricky in python and bash, although still doable.

One CLI command is an assignment for a team of one or two students.
Please note that you have to collaborate a lot with the team that implements your
invariant check command. If their program does not work well, then we can not test 
your program, then you do not get the score. You may see it as work in teams of
teams of students :)

Score ranges:

 1. passes the test cases: 5-6,
 2. passes manual invariant checks: 7-8,
 3. passes invariant fuzz tests: 9-10.

##  The basis

The basic RDX commands are provided to you as necessary building blocks.
These are three: 

 1. `rdx` for text and binary RDX manipulation,
 2. `brix` for RDX LSM/SST data file manipulation (lightweight db),
 3. `trix` for RDX to/from text manipulation (revision control).

You can use these as a library or as a CLI tool.
The interfaces and invariants of these commands are specified as well.

##  Commands to implement

### `fork` the revision control system

`fork` is a very basic git-inspired revision control system.
The text is stored in an RDX LSM database (trix, brix).
Merge is performed at the RDX level.
See data formats in `fork/` also all the `fork-*` command folders.

### `gatling` the swarm download system

`gatling` is a BitTorrent-inspired swarm download system.
Files are chopped into chunks, chunks get downloaded and shared by HTTP.
All the metadata is RDX.
Participants can modify the "torrent" and resync the changes.
See data formats in `gat/` also all the `gat-*` command folders.

### `concoin` the cryptocurrency

`concoin` is a very basic BitCoin-inspired cryptocurrency.
Wallet balances are kept as an RDX LSM database.
To contribute a new block, a proof-of-work is necessary.
See data formats in `con/` also all the `con-*` command folders.

### `redka` a Riak/Redis store

`redka` is a key-value store; if we agree to call Redis
a database then `redka` is also a database. Our main use case
is counters; imagine you count usage statistics for your
numerous distributed cloud clients in real time.
The general architecture is described in `redka/`, particular
commands are described in `redka-*` folders.
