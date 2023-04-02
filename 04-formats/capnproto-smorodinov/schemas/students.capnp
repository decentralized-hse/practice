@0xeefb9e5ef9b469af;

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

struct Students {
    students @0 :List(Student);
}
