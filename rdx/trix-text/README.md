#   Produces a text file out of RDX L

One way to store a versioned text in RDX is to use a Linear element.
It employs the Causal Tree / Replicated Growable Array data structure,
That is exactly what we need, except we have to flatten it to a single
string.

Invocation: `trix text A > T`.

##  Invariants

### Round trip 

````
    echo "Hello world!" > T
    touch E
    trix diff E T > D
    trix text D > T2
    cat T2
    Hello world!
````

### Diff/text invariant 

````
    trix text A > TA
    cp TA TB
    # edit TB
    trix diff TA TB > D 
    rdx merge A D > B
    trix text B > TB2
    diff TB TB2
````
