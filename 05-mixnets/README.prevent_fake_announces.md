## Preventing fake announces

A simple strategy to prevent routing table flooding (fake announces spam), based on proof of work principle.

### Problem
In our mixnet, each node contains a routing table, which is filled in when announcing from other nodes, 
we use this table to know through which node to send and resend messages. Here there is a problem of unscrupulous nodes 
that can overload the network by sending a large number of fake announcements, thereby increasing the routing tables 
at the nodes and thus reducing the network speed.

### Solve
We used the idea of Proof Of Work. Let's define hash happiness as having *k* zeros in a row at the beginning of the hash. 
In our network, the announcement is hashed upon receipt and forwarded to all neighbors except the sender, 
now we will forward it only if the hash from it is a happy one. In order to send the announcement to all other nodes, 
each node will have to calculate the hash so that it remains happy throughout the *diam* of applying 
the hash function to it, where `diam` is the diameter of the network.

So if the node wants to send an `announce_hash` as an announcement, it will need to select such `announce_hash` 
in order to:
* `sha256(announce_hash)`
* `sha256(sha256(announce_hash))`
* ...
* `sha256(sha256(...sha256(announce_hash)...)`

All these hashes were lucky, where the last hash is the application of the hash function _diam_ times.
To select such a `happy_hash`, the sender needs to calculate the order of _n^diam_ values, it makes spam more 
difficult, making it costly