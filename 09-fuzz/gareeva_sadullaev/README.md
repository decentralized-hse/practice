### Запустить

#### Установить Jazzer (arm64)


```shell
    wget https://github.com/CodeIntelligenceTesting/jazzer/releases/download/v0.22.1/jazzer-macos.tar.gz
```

Установить в свой локальный мавен-репозиторий:

```
mvn install:install-file -Dfile= ../jazzer_standalone.jar  -DgroupId=com.code_intelligence -DartifactId=jazzer -Dversion=0.22.1 -Dpackaging=jar
```

Запустить jar'ник 


```shell
java -jar path/to/jar pwd/students.bin
```
```shell
java -jar path/to/jar pwd/students.json
```


Запустить фазинг

Cобрать:

```shell
mvn clean install
```

Фазинг:

```
path/to/jazzer --cp=target/ds_hw_2-1.0-SNAPSHOT-jar-with-dependencies.jar --target_class=Fuzzer --target_method=fuzzerTestOneInput
```


Крутилось несколько часов, обнаружились ошибки такие:
```
java.lang.NullPointerException: Cannot invoke "java.util.List.size()" because "students" is null
        at Main.formatJsonToBin(Main.java:59)
        at Fuzzer.fuzzerTestOneInput(Fuzzer.java:31)
```

больше ошибок не обнаружилось, гарантировалось roundtrip гарантия на разных входных данных в json и bin файлах.

Также для большей уверенности я написал сам для себя скрипт который генерирует данные и смотрел на разных данных как оно себя ведет. Скрипт приложил рядышком.