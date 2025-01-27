#   RDX parse the JSON-like RDX into binary

Converts JSON-like RDX JDR into binary RDX.

Invocation: `rdx tlv J > A`.

##  Examples

##  Invariants

### Round-trip 

````
    rdx tlv J > A
    rdx jdr A > J2
    rdx eq J J2
````
