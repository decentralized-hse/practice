extern crate capnpc;

fn main() {
    capnpc::CompilerCommand::new()
        .output_path("src/")
        .src_prefix("schemas/")
        .file("schemas/students.capnp")
        .run()
        .unwrap();
}
