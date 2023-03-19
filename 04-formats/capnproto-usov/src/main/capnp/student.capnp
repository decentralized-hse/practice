@0xb54c1b3fe2c0b013;

using Java = import "/java.capnp";

$Java.package("com.hse");
$Java.outerClassname("Formats");

struct Student {
    name @0 :Text;
    login @1 :Text;
    group @2 :Text;
    practice @3 :List(UInt8);
    project @4 :Project;
    struct Project {
        repo @0 :Text;
        mark @1 :UInt8;
    }
    mark @5 :Float32;
}