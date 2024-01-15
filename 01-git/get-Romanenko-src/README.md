Пример запуска:

```bash
cargo run file.txt 4a840e62119929fbcb548969f2472c91e16cc3d3c58bd5ce078edacda22d3697
```

Пример вывода:

```text
warning: crate `get_Romanenko_src` should have a snake case name
  |
  = help: convert the identifier to snake case: `get_romanenko_src`
  = note: `#[warn(non_snake_case)]` on by default

warning: `get-Romanenko-src` (bin "get-Romanenko-src") generated 1 warning
    Finished dev [unoptimized + debuginfo] target(s) in 0.10s
     Running `target/debug/get-Romanenko-src file.txt 4a840e62119929fbcb548969f2472c91e16cc3d3c58bd5ce078edacda22d3697`
Hello, world!
```