#   Syncing data over WiFi mesh, LoRa, Meshtastic

Broadcast networks have an advantage when syncing shared
datasets: the updates can also be shared!
In this task, we will implement a data syncing protocol
generally following the Meshtastic random-repeat pattern, but
for shared data.

Our data model is the same as in `08-db`: packets up to 4096
bytes long, starting with one-byte *liter* and two 64-bit ids
(the packet's own *id* and the *reference*), some content
inside. Ids have the same `src-seq-off` structure.

Consistency requirements are the same: causal consistency.

 1. there must be no gaps in packets retrieved from the same
    source (e.g. to put `ab-48c` into the storage we must first
    put `ab-48b` and everything prior)
 2. there must be no causal gaps (i.e. if a packet refers to
    some prior packet, that prior packet must be present)

There are two types of messages: full packets and announces.
An announce only contains a number of ids thus advertising the
fact the sender has that data and everything prior, according to
the consistency requirements. The announce also serves as a
query: once we see a peer lacks some data, we broadcast it.
An announce is either a [version vector][v] or a fragment of it.

Differently from the [Meshtastic][m]'s receive-and-relay logic,
we avoid relaying full packets. Instead, we broadcast announces
as those are more compact. A peer that did not receive the
packet, but received our announce, responds with its announce,
so we can relay the packet knowing there is an interested party.

An implementation can read and write packets into some storage
(think [Chotki][c], use a stub), the storage can also notify it
of packets received elsewhere.

[c]: https://github.com/learn-decentralized-systems/Chotki
[v]: https://en.wikipedia.org/wiki/Version_vector
[m]: https://meshtastic.org/
