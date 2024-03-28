# Фаззинг решения capnproto-smorodinov

Алашеев Иван

## Собрать решение

```bash
cargo build
```

## Запустить на файле `example.bin`

```bash
cargo run example.bin
# получаем example.capnproto
cargo run example.capnproto
# получаем example.bin
```

## Запустить фаззер

```bash
cargo fuzz run fuzz_target_1
```
