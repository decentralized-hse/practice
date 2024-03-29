# Fuzzing

Все необходимые для фаззинга файлы были добавлены в PR. Для запуска нужно выполнить следующую команду из директории `protobuf-kogan-romanova`:
```bash
./jazzer_release/jazzer --cp=./protobuf-kogan/out/production/protobuf-kogan:./protobuf-kogan/lib/protobuf-java-3.21.12.jar:./protobuf-kogan/lib/protobuf-kotlin-3.21.12.jar:./protobuf-kogan/lib/kotlin-stdlib-1.6.0.jar:./protobuf-kogan/lib/annotations-13.0.jar:./protobuf-kogan/lib/kotlin-stdlib-common-1.6.0.jar:./protobuf-kogan/lib/kotlin-stdlib-jdk8-1.8.0.jar:./protobuf-kogan/lib/kotlin-stdlib-1.8.0.jar:./protobuf-kogan/lib/kotlin-stdlib-common-1.8.0.jar:./protobuf-kogan/lib/kotlin-stdlib-jdk7-1.8.0.jar:./jazzer_release/jazzer_standalone.jar --target_class=KotlinFuzzer
```