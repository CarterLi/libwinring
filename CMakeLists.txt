cmake_minimum_required(VERSION 3.10)

# set the project name
project(libwinring-demo)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wconversion")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-O3")

# add the executable
add_executable(bench test/bench.cpp)
add_executable(main test/main.cpp)

target_include_directories(bench PUBLIC
                           "include"
                           )
target_link_libraries(bench PUBLIC
                     "ntdll"
                     )
target_include_directories(main PUBLIC
                           "include"
                           )
target_link_libraries(main PUBLIC
                     "ntdll"
                     )
