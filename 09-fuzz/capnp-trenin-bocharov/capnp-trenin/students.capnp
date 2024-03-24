using Go = import "/go.capnp";
@0x85d3acc39d94e0f8;
$Go.package("student");
$Go.import("fuzz_capnp/student");


struct Student {
    name @0 :Text;
    login @1 :Text;
    group @2 :Text;
    practice @3 :List(UInt8);
    project @4 :Project;
    mark @5 :Float32;

    struct Project {
        repo @0 :Text;
        mark @1 :UInt8;
    }
}

struct Students {
    list @0 :List(Student);
}
