#   RDX file equivalence

Checks whether two files are equivalent, be they binary RDX or JSON-like RDX.

Invocation: `rdx eq A B`.
Returns 0 for equivalent, -1 otherwise.
Prints "equivalent" / "not equivalent" to stderr.

##  Invariants

### Self-quivalence

````
    rdx eq A A
````

### Binary is equivalent to its text form

````
    rdx jdr A > J
    rdx eq A J
````
