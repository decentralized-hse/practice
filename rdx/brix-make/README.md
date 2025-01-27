#   Convent an RDX document into its key-value form

Makes an RDX brick file with the specified content.

Invocation: `brix make A > B`.

##  Invariants

### Round trip 

````
    brix make A > B
    brix dump B > A2
    rdx eq A A2
````

