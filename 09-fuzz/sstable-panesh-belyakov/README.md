## sstable-panesh-belyakov
### Как запускать
#### Исправленное приложение
```(shell)
go build
./sstable-panesh-fuzzer <filename>
```

#### Фаззер
```(shell)
go test -fuzz=Fuzz
```

### Что исправил

1. Не было валидации формата входного файла вообще. Добавил

2. У меня в оригинальном решении падала сборка go по причине
```
./app.go:219:25: cannot use file (variable of type *os.File) as objstorage.Writable value in argument to sstable.NewWriter: *os.File does not implement objstorage.Writable (missing method Abort)
```
Добавил класс-обертку над os.File

3. Оставил panic только там, где ошибка от системы, а не от ввода, например, когда программа не смогла создать файл для вывода данных.

4. Приложение не выполняло идемпотентное round-trip преобразование, когда в Mark было float(-0). Запретил такой формат ввода.

### Результаты
Оставил фаззер включенным на ночь, на утро все еще гонялся, хвост логов:
```
fuzz: elapsed: 5h30m18s, execs: 74590167 (3542/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m21s, execs: 74601887 (3907/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m24s, execs: 74613736 (3950/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m27s, execs: 74625500 (3921/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m30s, execs: 74636153 (3551/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m33s, execs: 74647097 (3649/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m36s, execs: 74653331 (2077/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m39s, execs: 74658414 (1695/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m42s, execs: 74664091 (1892/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m45s, execs: 74670579 (2163/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m48s, execs: 74677978 (2467/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m51s, execs: 74685167 (2394/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m54s, execs: 74690838 (1891/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h30m57s, execs: 74698068 (2411/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m0s, execs: 74702027 (1320/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m3s, execs: 74706105 (1359/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m6s, execs: 74710591 (1495/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m9s, execs: 74713636 (1015/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m12s, execs: 74719433 (1932/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m15s, execs: 74724524 (1695/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m18s, execs: 74730716 (2066/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m21s, execs: 74750235 (6507/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m24s, execs: 74758812 (2859/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m27s, execs: 74767629 (2939/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m30s, execs: 74775730 (2701/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m33s, execs: 74782617 (2296/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m36s, execs: 74791000 (2794/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m39s, execs: 74798684 (2562/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m42s, execs: 74805058 (2125/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m45s, execs: 74812153 (2365/sec), new interesting: 79 (total: 183)
fuzz: elapsed: 5h31m48s, execs: 74819860 (2569/sec), new interesting: 79 (total: 183)
^Cfuzz: elapsed: 5h31m50s, execs: 74826321 (2664/sec), new interesting: 79 (total: 183)
PASS
ok  	sstable-panesh-fuzzer	19911.348s
```