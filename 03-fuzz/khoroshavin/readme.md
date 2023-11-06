Для фаззинга требуется хотя бы какой-то линукс (WSL сойдёт, вроде) и установленный cargo для Rust.

Запуск:
cargo afl build && cargo afl fuzz -i in -o out -- target/debug/btree-ershov