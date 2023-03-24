cmake_minimum_required(VERSION 3.9.1)
project (8086_decode)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

add_executable(8086_decode src/main.cpp)
