### Prerequisites:
Ставим maven и openjdk, если ещё нет
```
$ sudo apt install openjdk maven 
```
Для фаззинга нужно скачать и распаковать файлики с релизом Jazzer.
```
$ wget https://github.com/CodeIntelligenceTesting/jazzer/releases/download/v0.22.1/jazzer-linux.tar.gz
$ tar zxf jazzer-linux.tar.gz
```
Теперь нужно дабваить Jazzer в локальный maven репозиторий
```
$ mvn install:install-file -Dfile=/путь_до_исходников_jazzer/jazzer/jazzer_standalone.jar  -DgroupId=com.code_intelligence -DartifactId=jazzer -Dversion=0.22.1 -Dpackaging=jar
```

По идее можно переходить к сборке, если я ничего не забыл упомянуть на этом этапе.

### Сборка проекта:
```
$ mvn clean install -U
```
Должна появиться директория target с нашими .jar файлами для запуска.
Чтобы просто запустить конвертер, пишем:

```
$ java -jar target/04-formats-1.0-SNAPSHOT-jar-with-dependencies.jar ../ivanov.bin
```

### Фаззинг:

Чтобы запустить фаззер, нужно функции fail() закомментить ```exit(1)``` и раскомментить выбрасывание исключения (и всё пересобрать).
Запускаем фаззер командой:
```
$ /путь_до_исходников_jazzer/jazzer --cp=target/04-formats-1.0-SNAPSHOT-jar-with-dependencies.jar --target_class=Fuzzer --target_method=fuzzerTestOneInput
```