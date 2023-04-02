@0xce805f31c02b45a1;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("capn_student");

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
