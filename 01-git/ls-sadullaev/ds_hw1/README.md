### Как собрать

```shell
mvn clean install
```

Для теста лучше всего перейти в папку example. 

### Тестирование

```shell
cd example &&  java -jar ../01-git-1.0-SNAPSHOT-jar-with-dependencies.jar . 11e8c85b6ba3532ee79e4754bd0e7dbd454c0896086a45efdac2995f258be35a
```

Добавил в утилиту описание куда что писать чтобы получить желаемое.


Получаться должно что-то такое:

```
kopylov.txt
osetrov.md
```

Также вот примеры работы: 
```
muzaffar@MacBook-Pro-Muzaffar example % java -jar ../target/01-git-1.0-SNAPSHOT-jar-with-dependencies.jar . f7baf1bd1814f351b312e5adc0b7204bc8e455a22417190564cb4991345f945c
.commit
.parent/
```

Отрабатывает нормально. 
Пример брал с тех что были в других работах, работоспособность программы и обработки ошибок ориентировался по тем работам что были смерджены в главную ветку.
