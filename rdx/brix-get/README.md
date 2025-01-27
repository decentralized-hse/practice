#   Get RDX document out of a key-value BRIX

Given the ID, assembles an RDX document from key-value records.

Invocation: `brix get b0b-123 B > A`.

##  Invariants

### Round trip 

````
    brix make A > B
    brix get b0b-123 B > A2
    rdx eq A A2
````

