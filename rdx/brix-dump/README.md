#   BRIX file dump

Produces an RDX document out of a BRIX file.
If the key-value records refer to each other in non-tree fashion, the bundling is arbitrary.

Invocation: `brix dump B > A`.

##  Invariants

### Round trip 

````
    brix make A > B
    brix dump B > A2
    rdx eq A A2
````

