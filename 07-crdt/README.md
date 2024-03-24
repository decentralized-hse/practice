#   Problem 07: the RDX CRDT library

Our goal here is to create a format and a library for data
replication using state-of-the-art Replicated Data Types.
Replicated Data interchange format ([RDX][j]) is like protobuf,
but CRDT. Apart from [RPC][p] applications, one can use it for
data storage, distributed and asynchronous data exchange and in
other similar applications. RDX fully supports local-first,
offline-first and peer-to-peer replication, with no central
server required, as any two *replicas* can merge their data. By
installing RDX data types as merge operators in an LSM database
(leveldb, RocksDB, pebble, Cassandra, etc) one can effectively
have a CRDT database (which [Chotki][c] basically is).

We will implement *unified* CRDTs able to synchronize using
operations, full states or deltas. Types may imply [causal
consistency][x] of updates in matters of performance, but their
correctness does not depend on that. RDX data types are fully
commutative, associative and idempotent. Hence, immune to
reordering or duplication of updates.

The default syncing protocol (not described here) generally
relies on [version vectors][v]. Do not confuse that with [vector
clocks][r] used by Amazon Dynamo and similar systems. While
there are strong parallels, inner workings of VV and VC are not
identical.

There are seven assignments. For each data type one has to
create 10 functions implementing it. The 7th assignment is to
make a code generator, very much like `protoc`, using those
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

 1. last-write-wins variables (`I` for int64, `S` for string, `F`
    is float64, and `R` is [id64][i])
 2. counters, `N` increment-only uint64 and `Z` two-way int64
 3. maps (M), like key-value maps, where keys and values are `ISFR`
 4. sets (E), contain arbitrary `ISFR` elements
 5. arrays (L) of arbitrary `ISFR` elements
 6. version vectors (V)
 7. codegen

The format and the merge rules are as follows.

### `ISFR`

The last-write-wins register is the simplest data type to
implement. For a field, we only need a logical timestamp and the
value per se. A logical timestamp is a pair `{rev, src}` where
`rev` is the revision number and `src` is the id of the author.
For example, let's see how a bare (no TLV envelope) `I` int64
`-11` would look like, assuming it is the 4th revision of the
register autored by replica #5. The TLV would look like: `32 08
05 15` (hex) where `0x15` is a [zig-zag][g] encoded and zipped
`-11`, while `32 08 05` is a tiny [ToyTLV][t] record for a
zipped pair of ints, 4 (signed, zig-zagged, so `08`) and 5
(unsigned, so `05`). If we add a ToyTLV envelope, that becomes
`69 04 32 08 05 15` (type of record `I`, length 4, then the bare
part).

String values are simply UTF-8 strings; int64, float64 and id64
values get compressed using [`zip_int`][z] routines. Overlong
encodings are forbidden both for strings and for zip-ints! 

Merge rules for LWW are straighforward:

 1. higher revision wins
 2. in case of a tie, higher value wins (like bytes.Compare())
 3. in case of a tie, who cares, but higher replica id wins

### `NZ`

`N` are increment-only counters. Their TLV state is a sequence
of `U` records containing zipped uint64 pairs {val,src}, the
counter value and source replica id. Their merge operator is
per-replica `max` (as later versions are greater). Their native
value is the sum of all replica values.

`Z` are two-way counters (inc/dec). Their TLV format is a
sequence of `I` records each having `{rev,src}` metadata as
described in the `ISFR` section. One record corresponds to one
source, per-source merge rules are same as LWW. The native value
is the sum of all `I` values.

### `E`

Generic sets containing any `ISFR` elements. The TLV format is a
sequence of enveloped ISFR records. It can contain records with
negative version numbers. Those are tombstones (deleted
entries). For example, `-11` from the previous example would go
as `69 04 32 08 05 15`. Then, if replica #9 would remove that
entry, it will change to `69 04 32 07 09 15`. Here, the version
number changes `08` to `07` or 4 to -4, the author changes to 9.
The entries are sorted asc lexicographically (bytes.Compare).

### `M`

Generic maps, mapping any `ISFR` value to any other `ISFR`
value. The TLV format is a sequence of `ISFR` records, as
described in the `ISFR` section. Records go in key-value pairs,
except for tombstones (deleted entries) which go as keys, no
values. Tombstones have negative revision number, same as `E`.

Pairs are sorted in the ascending byte-order of their key
records (like `bytes.Compare()`). 

The merge strategy is last-writer-wins, in regard to values.

### `L`

Generic arrays store any `ISFR` elements. The TLV format is a
sequence of enveloped ISFR records. The order of the sequence is
`weave`-like, i.e. records and tombstones go as they appear(ed)
in the array. Deleted records change to tombstones, same as E.

### `V`

[Version vector][v] is a way to track dataset versions in a
causally ordered system. It is a vector of `seq` numbers, where
each `seq` is the version of the state as seen by each
respective replica. Alternatively, that is a map `{src: seq}`,
where `src` is the replica `id`. It is assumed, that we received
updates from replica `src` all the way up to `seq`.

Bare TLV for a version vector is a sequence of `V` records (yes,
`V` nested in `V`) each containing one id64 as a zipped seq-src
pair (see ZipUint64Pair). The sequence is sorted in the
ascenting order of record bytes, like `bytes.Compare()`.

The merge algorithm for version vectors is simple: take the
maximum `seq` for each `src`. Note that `seq=0` is distinct from
having no record.

##  Data type implementation

To fully implement an RDT one has to implement these 10
functions. The function name starts with the type name letter,
here we imply `I` last-write-wins int64.

````go
    // Xvalid verifies validity of a bare TLV record.
    // Any other function may assume the input is valid.
    func Ivalid(tlv []byte) bool 


    // Xstring converts a TLV representation into a string.
    func Istring(tlv []byte) (txt string) 

    // Xparse converts a string back into bare TLV.
    // Must round-trip with Xstring.
    func Iparse(txt string) (tlv []byte) 


    // Xtlv converts the native type into a TLV, zero metadata.
    func Itlv(i int64) (tlv []byte)

    // Xnative converts TLV into the native value.
    // Must round-trip with Xtlv.
    func Inative(tlv []byte) int64


    // Xdelta produces a TLV value that, once merged with
    // the old TLV value using Xmerge, will produce the new
    // native value using Xnative. Returns nil if none needed.
    // This function we need to *save changes* from a native
    // object/struct into RDX.
    func Idelta(tlv []byte, new_val int64) (tlv_delta []byte) 

    // Xmerge CRDT-merges several bare TLV values into the
    // resulting one. For example, given two I records
    // {3,8}15 and {4,1}44 will return {4,1}44 as version 4 is
    // newer than version 3.
    func Imerge(tlvs [][]byte) (tlv []byte) 

    // Xdiff produces a TLV delta given a TLV value and a
    // version vector of suspected changes (may skip this).
    func Idiff(tlv []byte, vvdiff VV) (tlv []byte)
````

##  Serialization format

We use the [ToyTLV][t] format for enveloping/nesting all data.
That is a bare-bones type-length-value format with zero
semantics. What we put into ToyTLV envelopes is integers,
strings, and floats. Strings are UTF-8, no surprises. Floats are
taken as raw bits and treated same as integers. id64 is stored
as a compressed pair of integers.

A note on integer compression. From the fact that protobuf
has about ten integer types, one can guess that things can
be complicated here. We use [ZipInt][z] routines to produce
efficient varints in a TLV format (differently from protobuf
which has a separate bit-level [LEB128][b] coding for ints). 

  - ZipUint64 packs an integer skipping all leading zeroes
  - ZipUint64Pair packs a pair of ints, each one taking 1,2,4 or
    8 bytes
  - ZipZagInt64 packs a signed integer using the zig-zag coding
  - ZipFloat64 packs a float (integers and binary fractions pack
    well)

id64 and logical timestamps get packed as pairs of uint64s. All
zip codings are little-endian.

##  Tests

The testing system for implementations is as follows: given a
stream of TLV records (stdin), merge them and output the result
as a string (stdout). Any permutation of records (including
duplication) must produce the same result. This test only
requires `Xmerge` and `Xstring` to be implemented. It is highly
recommended to share [ToyTLV][t] implementations. It also
recommended to check the inputs for correctness.

[c]: https://github.com/learn-decentralized-systems/Chotki/blob/main/ARCHITECTURE.md
[x]: https://en.wikipedia.org/wiki/Causal_consistency
[v]: https://en.wikipedia.org/wiki/Version_vector
[r]: https://www.educative.io/answers/how-are-vector-clocks-used-in-dynamo
[j]: https://en.wikipedia.org/wiki/RDX
[p]: https://en.wikipedia.org/wiki/Remote_procedure_call
[z]: https://github.com/learn-decentralized-systems/Chotki/blob/main/zipint.go
[g]: https://protobuf.dev/programming-guides/encoding/
[t]: https://github.com/learn-decentralized-systems/toytlv
[b]: https://en.wikipedia.org/wiki/LEB128
[i]: https://github.com/learn-decentralized-systems/Chotki/blob/main/id.go#L12
