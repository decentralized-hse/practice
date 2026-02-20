# Сборка
```
zig build-exe check.zig
```

# Запуск
```
./check <dir> <start_hash>
```

# Пример вывода корректного дерева (dir ./correct)
```
checking commit hash; commit_hash=eac6b33b5eeae8d1e88a3343fc00e8e9c3a5ab120198aecd5c39665852dd0016
checking commit hash; commit_hash=ebdd7b80aa4838dc4585af7f7d47b643024be479822f6c59b7c6313ce8322173
checking commit hash; commit_hash=ea57e88f130debd48188990f1175c6cd891ef11017c1145fbf07f39ec2503aa8
checking commit hash; commit_hash=86012759befd37636df5cc94ad673676a336616acd8b6156ae1bfdaf77465924
All is correct!
```

# Некорректное дерево
Может вернуться либо ошибка IncorrectHash, в случае неверного хеша в названии файла или директории, либо FileNotFound в случае несуществующего файла или директории
