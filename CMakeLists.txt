cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

project(qkd-ir)

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(demos)
add_subdirectory(tests)
