# git log

## Сборка

```bash
gcc main.c -o log
```


## Тестирование
В папке example есть коммиты на которых можно проверить корректность работы.

Ожидаемый вывод:

```bash
$ log 8f2b3b21335da1f9211eee0b514499d622c3b09a9bfd3dcede1e90f3633f06a6
Root: 14bab1779c8b86c15bec26e1fe28cf2a2e6d5ef4abf0ad030a9feb323507a789
Date: 24 Feb 2024 22:21:16 MSK

first commit
some text
```

```bash
$ log a1904686a1f37f3c7865193471b857902dbf203708f6598f8a26587e7b83ce64
Root: 377a6f550c2c0b32c38040b476b3d7f785370a98dba322e6c059bd8d4addce3e
Date: 24 Feb 2024 22:22:38 MSK

second commit
some text

Root: 14bab1779c8b86c15bec26e1fe28cf2a2e6d5ef4abf0ad030a9feb323507a789
Date: 24 Feb 2024 22:21:16 MSK

first commit
some text
```
