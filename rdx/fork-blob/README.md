#   Revision-controlled blobs

Storing blobs is a known git weakness. Let's try to implement 
an effective approach of managing and revision-controlling blobs. 
These are particular use cases we might be interested in:

 1. build artefacts (binaries),
 2. multimedia,
 3. system images/containers/etc.

We assume that binary artefacts may change gradually, i.e. it
makes sense to reuse some chunks. The problem was solved by rsync
long time ago and nothing prevents us from borrowing some code
from rsync. We add an abstraction layer to make things reusable.

Our target RDX manifest format for a single blob is:
````
    [
        50523:f976ac19c16949fc098966a870332a3865f3783d3c350de20fbd710aa9fcc87b
        81935:479770988cb3c3bff022cc405dd739dba99a5233ce3d47154fd51d7c7af62ecb
        61027:4bcec969d2b1da3b60d412baf90d9e85c0f144b6ac4a4d2270f24ad1d0f9023a
    ]
````
We use rsync rolling hashes to split blobs into chunks of approx. 64KB.
SHA256 hashes of chunks get included into the manifest.
If a file is *slightly* edited we can update separate chunks, 
leave the rest as-is.

Commands:

 1. `fork-blob` splits a blob into chunks, calculates SHA256 for the 
    chunks, produces the RDX manifest;
 2. `fork-up` given a manifest and a blob, uploads missing chunks
    into S3-compatible storage (the name of a chunk is its hash)
 3. `fork-down` given a manifest and an S3 storage URI, downloads 
    a blob chunk by chunk (preferably, in parallel)
 4. `fork-morph` given a manifest of a new version, S3 storage 
    URI and an old version of a blob, calculates what has to 
    be updated, downloads missing chunks, creates a new blob.

Testing: fuzz `fork-blob` + `fork-up` + `fork-down`,
`fork-blob` + `fork-up` + edit + `fork-blob` + `fork-up` + 
`fork-morph` (turns the old version into the new one).

Bonus track: upload manifests into S3, use their hash as a blob id,
download manifests on demand.

---

There is a way to download manifests gradually (e.g. RFC7574 Merkle
tree), not sure if anyone is ready for that :) Assuming blobs can
be terabyte-sized, the size of a manifest can be
    2**(40-16+5) = 2**29 = 512MB
which is substantial. If you volunteer to implement that, go ahead. 
