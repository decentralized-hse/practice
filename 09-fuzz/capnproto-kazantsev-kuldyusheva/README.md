Изначальная [реализация](https://github.com/decentralized-hse/practice/blob/65c172b4ac62b292fd0cc457416bf6ffffbceb05/04-formats/capnproto-kazantsev/capnproto-kazantsev.c%2B%2B).

### Пойманные ошибки:
```
    while(fread(&student, sizeof(Student), 1, in)) {
        dumpCapnproto(fd, student);
    }
```
Не было проверки, что чтение успешно. Проверка, что fread вернул > 0 некорректна, так как он возвращает количество прочитанных "студентов". В данном случае нужно сравнивать с 1.
Также при частичном прочтении "студента" значение не определено, поэтому я заменила на `ifstream.read()`.

```
    student.setName(bin_student.name);
    student.setLogin(bin_student.login);
    student.setGroup(bin_student.group);
    project.setRepo(bin_student.project.repo);
```
Не было валидации, что строка заканчивается на '\0'. Из-за этого был проезд по памяти.

### Запуск

Запуск fuzzing:
```
clang++ -g -fsanitize=address,fuzzer -L /usr/local/lib -lcapnp -lkj  fuzz.cc & ./a.out
```

Запуск программы:
```
clang++ -g -fsanitize=address -L /usr/local/lib -lcapnp -lkj  capnproto-kazantsev-kuldyusheva.c++ -o main && ./main "../ivanov.bin"
```