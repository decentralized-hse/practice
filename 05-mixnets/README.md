#   Drastijk: Dijkstrian mixnet

Here we propose an uber-paranoid message-passing mixnet where
participants are not aware of senders or recipients of messages
(which is commonplace), but also of the network's topology. The
only indormation a participant has is the following:

 1. entry points to connect to the network,
 2. public keys of their contacts.

We assume some sort of preferential-attachment network formation
which creates a topology of a constant diameter, although that
is not essential for the network's algorithms; this only ensures
reasonable path lengths. We assume that participants have vague
understanding of the network's size (e.g. 10000s of participants
of millions).

##  Routing announces

Participants do not announce their public keys to the network;
those are only known to their contacts. Instead, an initial
routing announce is: `A(0) = SHA256 ( pubkey + timestamp)`,
where the timestamp is rounded to the routing table update
period (say, 1 hour). Any subsequent announce (path length 1, 2
and so on) is like `A(i+1) = SHA256( A(i) )` Upon receiving an
announce, a node typically has no knowledge of the recipient or
the distance, but it can produce an `A(i+1)` annonce and forward
it to immediate contacts.

Note that having an `A(i)` announce, we can derive `A(i+k)` and
conclude that those paths are inferior: they lead to the same
node but take more hops. So, a node can make routing decisions
without knowing either destinations or distances.

Also note that announce propagation ends naturally as all the
nodes have some path to the destination (without knowing which
is which).

The message forwarding algorithm is straightforward: accept a 
message for `A(i+1)`, fetch `A(i)` from own records, forward the
message to the contact who sent `A(i)`.

##  Messages

Use ToyTLV, a simple type-length-value convention. A ToyTLV
message consists of:

 1. one-byte message type ([a-z] for short, [A-Z] for long
    messages, where short is up to 255 bytes of payload)
 2. message length (1 byte for short, 4 for long)
 3. message body of the specified length

A routing announce thus has a format 
    `a 32 sha256hash` (34b in all cases).
A message has a format 
    `M len sha256hash message_text` (34b or more).

We imply TCP-like transport between nodes (WebSocket etc).

##  Problems

 1. Design and implement delivery acknowledgements
 2. ...implement MTU and congestion control using (1)
 3. ...onion encyphering so a message looks different at every
    step (with a static message one can match different steps)
 4. ...some algorithm to try alternative paths (suppose some
    path is not working or gets congested); may use (1)
 5. ...a strategy to vary routing paths between periods
 6. ...a strategy to prevent routing table flooding (fake 
    announces); simple passive strategies are preferred
 7. ...other ways of sabotage and defences against them
 8. ...strategies for key rotation (Joe is malicious, I want
    to change my key so everyone but Joe gets the new one)
 9. ...strategies for contact sharing (friend-of-a-friend)
10. ...implement hypertext on top of such a network
    (use Markdown for document format), may use (2)
11. ...implement name system for such a network (like DNS)
12. ...implement simple e-mail for such network assuming
    there are full nodes (announced) and clients attached
    to full nodes; may use (11)

##  Examples

````
Current time: 2023-12-18 15:35:14.303890614 +0700 +07 m=+0.002900986
Current time (Unix): 1702888514
Current time (Unix nano): 1702888514303890614
Current time (start of the hour): 1702886400000000000
Public key: d4f94a31c97b8952368d0bdeec05c6b0b641e45a7a8e3035d273aecee7cab817
Hash A(0): 739547b5525eca23ef9e3e31391b7ad4bed7276ad09863721f0142362018f8ab
Hash A(1): a06a48b4c4f53be7446260d1d0b98adb8e5911bc430eec84439948feb519954c
Hash A(2): a848ff2a440c7d888957265a26ff24d78eb6720c7b4a05c74a1ddb20fff7962e
Announce A(0) ToyTLV: 6120739547b5525eca23ef9e3e31391b7ad4bed7276ad09863721f0142362018f8ab
Announce A(1) ToyTLV: 6120a06a48b4c4f53be7446260d1d0b98adb8e5911bc430eec84439948feb519954c
Message text: Hello world!
Message relayed for A(1): 6d2ca06a48b4c4f53be7446260d1d0b98adb8e5911bc430eec84439948feb519954c48656c6c6f20776f726c6421
````
