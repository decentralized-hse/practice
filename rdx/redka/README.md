#   Redka the key-value store

Redka is expected to be a fast mostly in-memory store, so all
files (SSTs and logs) are mmapped and generally expected to fit
in RAM. The architecture of a system is a basic LSM database
with strong influence of Riak's BitCask (see Petrov's book).

 1. the main process `redka-talk` is a single-thread poll-based
    TCP server. It accepts queries in RDX or JDR, sends 
    responses in the same format.

      - the main process maintains the WAL (write ahead log)
        where new writes reside; it also keeps a hash table to
        find new writes in the log. The hash table is of the
        most basic kind, `{id128 -> u32[4]}` so up to four
        writes to the same object can be tracked. In case a 5th
        write arrives to the same object, `redka-talk` merges
        all five, puts the result into the log, updates the
        index. The log is RDX. The hash table is `abc/HASHx.h`.
        For a basic poll-based TCP server see `abc/TCP.h`,
        `abc/POLL.h` and `abc/test`.

      - older writes reside in SST files, very much like in
        a regular LSM database. To answer a query, `redka-talk`
        merges entries from the SST files (see `BRIX`) and
        entries from the log.

      - once a log chunk reaches the size limit, `redka` starts
        a new one; the total size of all chunks can not exceed
        4GB. Older chunks are deleted once the data is moved
        to SST files.

 2. the compaction process `redka-compact` converts complete
    log chunks into SST files and merges SST files into bigger
    SST files, as every LSM database is doing. The goal is to
    keep the number of SST files under some limit. `redka-talk`
    and `redka-compact` communicate through a pipe.

 3. the wire protocol for `redka-talk` is RDX or JDR. JDR uses
    newlines to separate different requests and responses.

     1. `b0b-123` (a reference) is a query of a specific object
        if sent by a client; an id of a newly created object
        if sent by the server;
     2. `{@b0b-123 name:"Alice" address:"Wonderland"}`
        (a PLEX object, in this case an Eulerian set) is either 
        a response if sent by the server or a write if sent by
        a client; writes may use versioning in the
        usual RDX way, e.g. `{@b0b-123 address@2:"Tea Party"}`
        New object writes have no id: `{name:"Mad Hatter"}`
     3. So, every query is responded with a PLEX object or 
        `RDKAnone`; every write is responded with the id of the
        edited/created object. Any unclear message is responded
        with `RDKAbad`; on a parsing error `RDXbad`, connection
        closed.

 4. any bugs found in `abc`, `rdx` or `brix` should be reported
    or (better) fixed.

 5. the stress test would compare `redka` results against
    `std::map<id128, rdx_string>`. The fuzz test would compare
    results to a regular sequential RDX merge (the constants
    will be adjusted to trigger all the mechanics early, e.g.
    max log chunk of 64 bytes).
