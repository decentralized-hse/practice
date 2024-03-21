Весь код написан в файле `xml-nasonkov.cpp`, в файлах `tinyxml2.h` и `tinyxml2.cpp` -- сторонняя библиотека.

Компиляция: `g++ -std=c++17 xml-nasonkov.cpp tinyxml2.cpp -o xml-nasonkov`

Генерируемый из файла-образца `ivanov.bin` XML можно посмотреть в файле `ivanov.xml`

clang++ -std=c++17 -g -fsanitize=address,fuzzer fuzz.cpp xml-nasonkov.cpp