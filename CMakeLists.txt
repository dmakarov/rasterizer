cmake_minimum_required(VERSION 3.3)
project(rasterizer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/canvas.cpp
    src/canvas.hpp
    src/main.cpp)

include_directories(/usr/local/include)

add_executable(rasterizer ${SOURCE_FILES})