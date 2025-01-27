#   Merging RDX documents

merged document

Invocation: `rdx merge A B > C`

##  Examples

````
    cat A
    {1:"one"}
    cat B
    {2:"two"}
    rdx merge A B | rdx j
    {1:"one",2:"two"}
````

##  Invariants

### Associativity

````
    rdx merge A B > AB 
    rdx merge AB C > ABC
    rdx merge B C > BC
    rdx merge A BC > ABC2
    rdx eq ABC ABC2
````

### Commutativity 

````
    rdx merge A B > AB 
    rdx merge B A > AB2
    rdx eq AB AB2
````

### Idempotence

````
    rdx merge A A > A2 
    rdx eq A A2
````

### Diff/merge invariant

````
    rdx diff A B > D
    rdx merge A D > B2
    rdx eq B B2
````

