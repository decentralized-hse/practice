### Сборка

```shell
gradle clean build
```

Уже собранное приложение ```ls-karachik-0.1.jar``` лежит в папке ```example```.

### Тестирование

```shell
cd example
java -jar ls-karachik-0.1.jar . 71feb562937803d2621cc2b793ad75f349fbb27b2da5b67c1269bd2e8a4a6f7f
```


Пример вывода:
```
example> java -jar ls-karachik-0.1.jar . 71feb562937803d2621cc2b793ad75f349fbb27b2da5b67c1269bd2e8a4a6f7f
.parent/
.commit
README
hello.txt
osetrov.txt
subdir/
subdir/osetrov.md
subdir/subsubdir/
subdir/text.txt
```
