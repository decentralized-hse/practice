cmake_minimum_required(VERSION 3.10)

# Установите имя проекта
project(ConPick)

# Установите стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Укажите путь к заголовочным файлам nlohmann-json
include_directories("/opt/homebrew/opt/nlohmann-json/include")

# Добавьте исполняемый файл
add_executable(con_pick con_pick.cpp)
