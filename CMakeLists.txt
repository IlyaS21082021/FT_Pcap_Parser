cmake_minimum_required(VERSION 3.5)

project(parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(parser main.cpp
    parser_structs.h parser.cpp parser.h)

include_directories(parser ${CMAKE_SOURCE_DIR})
