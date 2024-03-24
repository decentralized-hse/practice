# git check

Алашеев Иван

Утилита проверяет валидность хешей и формат файлов с описанием директории.

Как собрать и запустить утилиту:
```bash
mkdir build; cd build
cmake ..
cmake --build .
./check-alasheev <path/to/directory> <root-hash>
```

`<path/to/directory>` - директория с файловой системой

`root-hash` - хеш корневой директории

Запустить на примере из репозитория:
```bash
./check-alasheev ../../example/ f21ad9b137583c2d87ca8cf3a8c14934076adf50f96f465204262315e7bfeb87
```
