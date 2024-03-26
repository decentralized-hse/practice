Protobuf Code is generated from this directory by the command

```powershell
protoc --proto_path=. --java_out=src/main/kotlin --kotlin_out=src/main/kotlin ./struct.proto
```

(protoc version `3.21.12`)

Usage: open this directory in the terminal, then
```bash
java -classpath ./out/production/protobuf-kogan:./lib/protobuf-java-3.21.12.jar:./lib/protobuf-kotlin-3.21.12.jar:./lib/kotlin-stdlib-1.6.0.jar:./lib/annotations-13.0.jar:./lib/kotlin-stdlib-common-1.6.0.jar:./lib/kotlin-stdlib-jdk8-1.8.0.jar:./lib/kotlin-stdlib-1.8.0.jar:./lib/kotlin-stdlib-common-1.8.0.jar:./lib/kotlin-stdlib-jdk7-1.8.0.jar MainKt ../ivanov.bin
```
