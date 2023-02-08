#   BRIX batch syncing utility

As an LSM engine, BRIX creates stack of SST files containing
sorted key-value records. Suppose, replica A has a stack 
`a b c d e f` while replica B has an SST stack `a b c g h i`.
Note that `a`, `b`, `c` etc are all SHA256 hashes. 

Why do stacks differ? Well, if we only sync replicas in batch
mode then they will obviously differ. Some writes went to A and
others to B. In case we sync them in real time, still the 
order of writes will vary. So, different subsets of writes will
go into different SST files. As a result, the data *might* be
the same but the files and hashes will be different.

We want to ensure replicas are *identical* (hashes match).

 1. `brix-sync` will run on both ends, communicating through
    TCP or an SSH tunnel (the latter allows one `brix-sync`
    to start another so they talk through stdin/stdout)
 2. It will take all the local unsynced SSTs (e.g. `def` for A)
    and start merging them, writing the results to a temporary
    buffer. Everything written to the buffer is also sent to
    the other side. 
 3. The other side merges the incoming records with the ones 
    in its own temporary buffer, flushing the result into the 
    new SST file. That way, the temporary buffer can have 
    a fixed size, as it is appended on one end and consumed on 
    the other end, at the same time. That way, we use a fixed
    amount of RAM which is proportional to the size of data 
    in flight (in the network).
 4. In the end of a run, the resulting SST files on both sides
    must be identical (same hashes).

