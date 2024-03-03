## Как собирала?

```
clang++ -g -fsanitize=fuzzer,address -I /usr/include/sqlite3.h -lsqlite3 fuzz_me.cc
```

## Как запускала?

```
./a.out -max_len=10000
```

## Замечания

- Код не поддерживает параллельный фаззинг, потому что сырые данные пишутся в файлик, сконвертированное тоже записывается .sqlite файлик

- Фаззила два часа у себя на машинке

- В файле `sqlite-gvasalia-kazantseva.h` лежит код прошлого года разбитый на функции `ConvertBinaryDataIntoSqlite` и `ConvertSqliteDataIntoBinary`.

- В файлике `precheck.h` лежат функции по проверке данных, прежде, чем их конвертировать как-то

## Внесённые изменения

- Добавлены функции `GetNewSize`, `GetNewSize`, так как если строка с данными содержит ', то при создании sql запроса нужно их удваивать, чтобы они появились

```cpp
size_t GetNewSize(const char* data, size_t size) {
    size_t count = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\'') {
            ++count;
        }
    };
    return count;
}


void GetNewSize(const char* oldData, char* newData, size_t size) {
    size_t j = 0;

    for (size_t i = 0; i < size; ++i) {
        if (oldData[i] == '\'') {
            newData[j] = '\'';
            ++j;
            newData[j] = '\'';
            ++j;
        } else {
            newData[j] = oldData[i];
            ++j;
        }
    }

    std::string end = "', '";
    for (size_t i = 0; i < end.size(); ++i) {
        newData[j++] = end[i];
    }
    newData[j++] = '\0';
    return;
}

```

- Соответсвенно добавлены такие строки, чтобы создать новый буфер с удвоенными ', если необходимо
- Также в случае с не нуллтерминированными строками спокойно можно было добавить ', ' , однако если нуллтерминированные, то при конкатенации с query их нужно еще ручками дописать
```cpp
    std::string end = "', '";
    size_t formattedStringSize = 32 + GetNewSize(record.name, 32) + end.size() + 1;
    char formattedStringName[formattedStringSize];
    PrepareStringForSql(record.name, formattedStringName, 32);
    query += formattedStringName;

    if (std::string(formattedStringName).size() + 1 != formattedStringSize) {
        query += "', '";
    }
```

- Добавлена следующая строка приводящая к memory-leak

```
sqlite3_finalize(stmt);
```

- Изменила храненое float оценок в таблице на строки

```cpp
rc = sqlite3_exec(db, "CREATE TABLE students (name TEXT, login TEXT, group1 TEXT, practice TEXT, repo TEXT, mark_project INTEGER, mark TEXT); ", NULL, NULL, NULL);
```

- Изменила точность на double, так как иначе не хватало точности для обратной конвертации для значений вроде: 8.1791e-41

```cpp
    std::ostringstream out;
    out << std::scientific << std::setprecision(std::numeric_limits<double>::max_digits10) << double(record.mark);
    std::string markStr = out.str();
    query += markStr;
    query += "');";
```

- Добавила таких штук для проверки корректности работы с бд

```cpp
    char *zErrMsg = 0;
    rc = sqlite3_exec(db, query.c_str(), NULL, NULL, &zErrMsg);
    if( rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
```
