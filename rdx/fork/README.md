# `fork` the revision control system

`fork` is roughly based on the `git` ideas.
All the data is kept in a `brix` store, all the merge is RDX.
A repo lives in `project_root/.fork`.

The root document format:

````
    {
        "README.md":  b0b-1,
        "brix-dump":  { "README.md": b0b-2 },
        "brix-get":  { "README.md": b0b-3 },
        "brix-make":  { "README.md": b0b-4 },
        "brix-merge":  { "README.md": b0b-5 },
        "con":  { "README.md": b0b-6 },
        "fork":  { "README.md": b0b-7 },
        "gat":  { "README.md": b0b-8 },
        "rdx-diff":  { "README.md": b0b-9 },
        "rdx-eq":  { "README.md": b0b-a },
        "rdx-jdr":  { "README.md": b0b-b },
        "rdx-merge":  { "README.md": b0b-c },
        "rdx-tlv":  { "README.md": b0b-d },
        "trix-diff":  { "README.md": b0b-e },
        "trix-text":  { "README.md": b0b-f },
    }
````

Here, `b0b-123` are 128 bit ids of RDX L elements containing the text of a file.
The elements are created by the `trix` command.

The commit naming and tracking is done at the BRIX level, using hashes.

