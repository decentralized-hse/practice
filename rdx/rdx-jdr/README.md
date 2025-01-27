#   RDX JDR: the JSON-like text serialization

Converts binary RDX into JSON-like RDX JDR.

Invocation: `rdx jdr A > J`.

##  Examples

##  Invariants

### Round-trip 

````
    rdx jdr A > J
    rdx tlv J > A2
    rdx eq A A2
````

### Binary is equivalent to its text form

````
    rdx jdr A > J
    rdx eq A J
````
