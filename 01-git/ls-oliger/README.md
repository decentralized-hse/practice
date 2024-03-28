# 01-git
---

> Язык: C++ | Задача: ls.

---
### Сборка
```shell
g++ -o ls-oliger main.cpp
```
### Формат ввода
```
./ls-oliger <dir> <hash>
```
---
### Пример работы

Для демонстрации работы использовал `examples.zip`, который был указан в нескольких других PR.

```shell
(base) nickoliger@Nicks-Air ls-oliger % ./ls-oliger . f2e827d2ab627239199f639c475c7625a013fe2a8afd6696519329fda2977209
.commit
.parent/
README
hello.txt
osetrov.txt
subdir/
subdir/osetrov.md
subdir/text.txt
```

Пример обработки ошибок, если файла не существует:
```shell
(base) nickoliger@Nicks-Air ls-oliger % ./ls-oliger . 0dc5d308e4552f2e621391e5ec4b9de85c0575a54557393d4108f413fdff7b56
Could not open file. Details:
Hash: 0dc5d308e4552f2e621391e5ec4b9de85c0575a54557393d4108f413fdff7b56
Prefix:
Caught an error during runtime. See the info below:
Incorrect input (2)
```
---