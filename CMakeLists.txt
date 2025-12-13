cmake_minimum_required(VERSION 4.0)
project(cappuccino)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(cappuccino main.cpp
        utils.cpp
        utils.h
        Token.cpp
        Token.h
        AbstractSyntaxTree.cpp
        AbstractSyntaxTree.h
        Parser.cpp
        Parser.h
        CodeGen.h
        CodeGen.cpp
)
