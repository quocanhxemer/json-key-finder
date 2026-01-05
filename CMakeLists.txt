cmake_minimum_required(VERSION 3.16)
project(find_json_key LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FIND_KEY_NATIVE "Enable -march=native" ON)

add_library(find_json_key_warnings INTERFACE)
target_compile_options(find_json_key_warnings INTERFACE -Wall -Wextra -Wpedantic)

add_library(find_json_key STATIC
    src/findkey.cpp
    src/matcher_scalar.cpp
    src/mmap_file.cpp
)
target_include_directories(find_json_key
    PUBLIC include 
    PRIVATE src
)
target_link_libraries(find_json_key PUBLIC find_json_key_warnings)

if(FIND_KEY_NATIVE)
    target_compile_options(find_json_key PUBLIC -march=native)
endif()

add_executable(findkey_scalar
    src/main.cpp
)
target_include_directories(findkey_scalar PRIVATE include)
target_link_libraries(findkey_scalar PRIVATE find_json_key)
