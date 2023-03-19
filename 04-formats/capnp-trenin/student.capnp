using Go = import "/go.capnp";
@0x85d3acc39d94e0f8;
$Go.package("student");
$Go.import("foo/student");

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
