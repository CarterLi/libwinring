cmake_minimum_required(VERSION 3.10)

# set the project name
project(libwinring-demo)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(bench test/bench.cpp)
add_executable(main test/main.cpp)

target_include_directories(bench PUBLIC
                           "include"
                           )
target_include_directories(main PUBLIC
                           "include"
                           )
