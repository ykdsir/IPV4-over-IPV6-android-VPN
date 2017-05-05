cmake_minimum_required(VERSION 3.6)
project(4over6_server)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(4over6_server ${SOURCE_FILES})