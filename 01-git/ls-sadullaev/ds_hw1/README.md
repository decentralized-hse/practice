### Как собрать

```shell
mvn clean install
```

Для теста лучше всего перейти в папку example. 

### Тестирование

```shell
cd example &&  java -jar path/to/my/jar-file/01-git-1.0-SNAPSHOT-jar-with-dependencies.jar . 71feb562937803d2621cc2b793ad75f349fbb27b2da5b67c1269bd2e8a4a6f7f
```

Добавил в утилиту описание куда что писать чтобы получить желаемое.


Получаться должно что-то такое:

```
muzaffar@MacBook-Pro-Muzaffar example % java -jar /Users/muzaffar/Documents/Code/java/ds_hw1/target/01-git-1.0-SNAPSHOT-jar-with-dependencies.jar . 71feb562937803d2621cc2b793ad75f349fbb27b2da5b67c1269bd2e8a4a6f7f
.parent/
.commit
README
hello.txt
osetrov.txt
subdir/
subdir/osetrov.md
subdir/subsubdir/
subdir/text.txt
muzaffar@MacBook-Pro-Muzaffar example % cd ..
```
