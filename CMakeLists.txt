cmake_minimum_required(VERSION 3.10)
project(problem_036_dynamic_bitset CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(code main.cpp)

if (MSVC)
  target_compile_options(code PRIVATE /O2)
else()
  target_compile_options(code PRIVATE -O2 -Wall -Wextra -Wshadow -Wconversion -Wno-sign-conversion)
endif()
