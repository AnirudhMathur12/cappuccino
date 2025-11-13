cmake_minimum_required(VERSION 4.0)
project(cappuccino)

set(CMAKE_CXX_STANDARD 20)

add_executable(cappuccino main.cpp
        utils.cpp
        utils.h
        Token.cpp
        Token.h)
