# git check by Nadezhda Filippova

В папке correct_example находится пример корректного дерева гита

Пример запуска из папки correct_example
```bash
go run ../check.go subdir 99682225f8eeb6a007bed389c124c659f596a794d556b4c865c99995c5f03b98 
```

Получим вывод
```bash
Tree is correct
```

Пример запуска для корневой директории из папки correct_example
```bash
go run ../check.go . 99682225f8eeb6a007bed389c124c659f596a794d556b4c865c99995c5f03b98 
```

Получим вывод
```bash
Tree is correct
```

В папке incorrect_example находится пример некорректного дерева гита

Пример запуска из папки incorrect_example
```bash
go run ../check.go subdir 99682225f8eeb6a007bed389c124c659f596a794d556b4c865c99995c5f03b98 
```

Получим вывод
```bash
Incorrect tree, file with hash e21d3a526d08ae5fac7bd69455022e2fec66b051641a1bcbbee7744e3205b02 can't be read 
```
