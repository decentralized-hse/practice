# Сборка
```
cargo build
```
# Запуск
Программа принимает на вход stdin tlv записи и возвращает результат в виде строки в stdout
*Пример:*
```
cat ../test_data/N0.tlv | cargo run --
```
P.S. two-way counters (`Z`) can be implemented after implementation of LWW (`FIRST`), type `I` in particular.