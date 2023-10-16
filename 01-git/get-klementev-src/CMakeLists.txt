cmake_minimum_required(VERSION 3.23)
project(get_klementev)

set(CMAKE_CXX_STANDARD 20)

add_link_options(-I/opt/ssl/include/ -L/opt/ssl/lib/ -lcrypto)

add_executable(get-klementev get-klementev.cpp FolderNode.h ObjType.h LineInfo.h Sha256HashChecker.h)
