# BASON Codec in Rust

A ready-to-use Rust Cargo project for **Assignment 1: BASON Codec**.

This project implements:

- BASON record encode / decode / decode-all
- RON64 encode / decode
- path utilities
- strictness validation
- JSON ↔ BASON conversion
- flatten / unflatten
- `basondump` CLI
- unit and integration tests

## Project layout

```text
bason-codec-rust/
├── Cargo.toml
├── README.md
├── src/
│   ├── lib.rs
│   └── main.rs
├── tests/
│   ├── codec.rs
│   └── strictness.rs
└── examples/
    └── sample.rs
```

## BASON spec alignment

This implementation is aligned to the uploaded BASON spec version 0.1:

- short layout: `tag | lengths | key | value`
- long layout: `tag | val_len(4B LE) | key_len(1B) | key | value`
- numbers are stored as **UTF-8 text**
- array indices use **RON64**
- strictness bits are implemented from the spec bitmask

## Build

```bash
cargo build
```

## Run tests

```bash
cargo test
```

## Run the CLI

Pretty-print BASON file:

```bash
cargo run --bin basondump -- ./sample.bason
```

JSON output:

```bash
cargo run --bin basondump -- ./sample.bason --mode json
```

Annotated hex output:

```bash
cargo run --bin basondump -- ./sample.bason --mode hex
```

Validate strictness:

```bash
cargo run --bin basondump -- ./sample.bason --strictness 511
```

Flatten decoded record tree:

```bash
cargo run --bin basondump -- ./sample.bason --flatten
```

Unflatten stream back to nested form:

```bash
cargo run --bin basondump -- ./sample.bason --unflatten
```

## Notes

- `bason_unflatten` reconstructs arrays heuristically by checking whether sibling path segments are a contiguous minimal RON64 range (`0..n-1`). This matches the BASON spec well for ordinary JSON data, but flat paths are inherently ambiguous if object field names themselves look exactly like array indices.
- In this environment I could not run `cargo test`, because the Rust toolchain is not installed in the container. The project was assembled carefully to match the uploaded BASON documents, but you should run `cargo test` locally after unpacking.
