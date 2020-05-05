cmake_minimum_required(VERSION 3.10)
project(diplom)

set(CMAKE_CXX_STANDARD 17)

add_executable(diplom main.cpp segment_tree.h simulator.h simulator.cpp segment_tree.cpp parser.h)
