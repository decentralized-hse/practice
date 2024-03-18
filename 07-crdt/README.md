#   Problem 07: the RDX CRDT library

Our goal here is to create a format and a library for data
replication using state-of-the-art Replicated Data Types.
Replicated Data interchange format ([RDX][j]). That is like
protobuf, but CRDT. Apart from [RPC][p] applications, one can
use it for data storage, distributed or asynchronous data
exchange and in other similar applications. RDX is able of
peer-to-peer replication, with no central server, as any two
replicas can merge their data. Whether *any* two versions of an
object can be merged still depends on the data type. Some RDTs
require causal consistency. Others are completely unkillable and
can merge in any conditions. By installing these data types as
merge operators in an LSM database (leveldb, RocksDB, pebble,
Cassandra, you name it) one can effectively have a CRDT database
(which [Chotki][c] basically is).

We will implement *unified* CRDTs able to synchronize using
operations, full states or deltas. Some types rely on [causal
consistency][x], others don't. The syncing protocol (not
described here) generally relies on [version vectors][v]. Not to
confuse with [vector clocks][r] used by Amazon Dynamo and
similar systems. (While there are strong parallels, inner
workings of VV and VC are not identical)

There are seven assignments. For each data type one has to
create 10 functions implementing it. The 7th assignment is to
make a code generator, very much like `protoc`, using those 10
functions, to load object from a database or store the changes
back. See the [Chotki][c] document section "Formal model" for
the general understanding of objects and fields, packets and
ops, and ids addressing them.

Permitted languages are:
 1. C
 2. C++
 3. Rust
 4. Python
 5. Go
 6. node.js
 7. .NET
 8. Java

##  Data types

Our objects can have fields of the following CRDT types. Each
type is named by a letter. 

 1. last-write-wins variables (`I` int64, `S` string, `F`
    float64, `R` id64)
 2. counters, `C` those requiring causally consistent update and
    `U` those unkillable (int64 in both cases)
 3. maps (M), like key-value maps, where each cell is an LWW
 4. sets (E), contain arbitrary `ISFR` elements
 5. arrays (L) of arbitrary `ISFR` elements
 6. version vectors (V)
 7. codegen

The format and the merge rules are as follows.

### `ISFR`

The last-write-wins register is the simplest data type to
implement. For a field, we only need a logical timestamp and the
value per se. A logical timestamp is a pair `{v, src}` where `v`
is the number of the version and `src` is the id of the author.
For example, a bare (no envelope) int `-11`, the 4th revision by
replica N5, would look in TLV like: `32 04 05 15` where `0x15`
is a [zig-zag][g] encoded and zipped `-11`. `32 04 05` is a tiny
ToyTLV record for a zipped pair of ints, 4 and 5.

String values are simply UTF-8 strings; int64, float64 and id64
values get compressed using [`zip_int`][z] routines. Overlong
encodings are forbidden both for strings and for zip-ints! 

Merge rules for LWW are straighforward:

 1. higher revision wins
 2. in case of a tie, higher value wins (like bytes.Compare())
 3. in case of a tie, who cares, but higher replica id wins

### `CU`

### `M`

### `E`

### `L`

### `V`

##  Data type implementation

````go
    func Ivalid(tlv []byte) bool 

    func Istring(tlv []byte) (txt string) 
    func Iparse(txt string) (tlv []byte) 

    func Itlv(i int64) (tlv []byte) 
    func Iplain(tlv []byte) int64 

    func Idelta(tlv []byte, new_val int64) (tlv_delta []byte) 
    func Imerge(tlvs [][]byte) (tlv []byte) 
    func Idiff(tlv []byte, vvdiff VV) []byte 
````

##  Serialization format

ToyTLV
ZipUint64  our varint format
From the fact that protobuf has about a dozen integer types, one can guess that things can be complicated)
ZipUint64Pair

[c]: https://github.com/learn-decentralized-systems/Chotki/blob/main/ARCHITECTURE.md
[x]: https://en.wikipedia.org/wiki/Causal_consistency
[v]: https://en.wikipedia.org/wiki/Version_vector
[r]: https://www.educative.io/answers/how-are-vector-clocks-used-in-dynamo
[j]: https://en.wikipedia.org/wiki/RDX
[p]: https://en.wikipedia.org/wiki/Remote_procedure_call
[z]: https://github.com/learn-decentralized-systems/Chotki/blob/main/zipint.go
[g]: https://protobuf.dev/programming-guides/encoding/
