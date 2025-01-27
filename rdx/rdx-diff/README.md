#   RDX diff 

Produces an RDX patch that transforms RDX A into RDX B if merged.

Invocation: `rdx diff A B`.

##  Examples

````
    $ cat A
    {1:"one"}
    $ cat B
    {1:"one",2:"two"}
    $ rdx diff A B | rdx jdr
    {2:"two"}
````

##  Invariants

### Diff/merge invariant 

````
    rdx diff A B > D
    rdx merge A D > B2
    rdx eq B B2
````

