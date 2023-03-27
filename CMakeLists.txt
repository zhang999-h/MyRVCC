cmake_minimum_required (VERSION 3.1)

project(MyRVCC)

add_executable(MyRVCC src/main.cpp src/codegen.cpp src/tokenize.cpp src/parse.cpp a.out
            src/error.cpp)  