# Сборка
```
zig build-exe check.zig
```

# Запуск
```
./check <path> <root_hash>
```

# Пример вывода корректного дерева (dir ./correct)
```
commit path: correct/61ec58ece347e6ac22f5227c642410bfad51e33bd2ca57e1f9d5b85502fa4ddf
hex of file hash:61ec58ece347e6ac22f5227c642410bfad51e33bd2ca57e1f9d5b85502fa4ddf
file name: 61ec58ece347e6ac22f5227c642410bfad51e33bd2ca57e1f9d5b85502fa4ddf
commit path: correct/ebdd7b80aa4838dc4585af7f7d47b643024be479822f6c59b7c6313ce8322173
hex of file hash:ebdd7b80aa4838dc4585af7f7d47b643024be479822f6c59b7c6313ce8322173
file name: ebdd7b80aa4838dc4585af7f7d47b643024be479822f6c59b7c6313ce8322173
commit path: correct/ea57e88f130debd48188990f1175c6cd891ef11017c1145fbf07f39ec2503aa8
hex of file hash:ea57e88f130debd48188990f1175c6cd891ef11017c1145fbf07f39ec2503aa8
file name: ea57e88f130debd48188990f1175c6cd891ef11017c1145fbf07f39ec2503aa8
commit path: correct/86012759befd37636df5cc94ad673676a336616acd8b6156ae1bfdaf77465924
hex of file hash:86012759befd37636df5cc94ad673676a336616acd8b6156ae1bfdaf77465924
file name: 86012759befd37636df5cc94ad673676a336616acd8b6156ae1bfdaf77465924
All is correct!
```

# Некорректное дерево
Может вернуться либо ошибка IncorrectHash, в случае неверного хеша в названии файла или директории, либо FileNotFound в случае несуществующего файла или директории
