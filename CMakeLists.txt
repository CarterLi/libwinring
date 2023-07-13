cmake_minimum_required(VERSION 3.10)

set (CMAKE_SYSTEM_VERSION 10.0 (latest installed version))
set (CMAKE_WINDOWS_KITS_10_DIR "C:/Program Files (x86)/Windows Kits/10")

# set the project name
project(libwinring-demo)

# specify the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_definitions(_WIN32_WINNT=_WIN32_WINNT_WIN10)
add_compile_definitions(NTDDI_VERSION=NTDDI_WIN10_NI)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--tsaware -Wl,--build-id -Wl,--subsystem,console:6.1,--major-os-version,10,--minor-os-version,0")
link_libraries("ntdll")

# add the executable
include("test/CMakeLists.txt")
include("demo/CMakeLists.txt")
include_directories("include")
