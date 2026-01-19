cmake_minimum_required(VERSION 3.16)
project(find_json_key LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FIND_KEY_NATIVE "Enable -march=native" ON)

add_library(find_json_key_warnings INTERFACE)
target_compile_options(find_json_key_warnings INTERFACE -Wall -Wextra -Wpedantic)

add_library(find_json_key STATIC
    src/core/findkey.cpp

    src/io/mmap_file.cpp

    src/teddy/teddy_common.cpp

    src/matchers/matcher_scalar.cpp
)
target_include_directories(find_json_key
    PUBLIC include 
    PRIVATE src
)
target_link_libraries(find_json_key PUBLIC find_json_key_warnings)

if(FIND_KEY_NATIVE)
    target_compile_options(find_json_key PUBLIC -march=native)
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mssse3" COMPILER_SUPPORTS_MSSSE3)

if(COMPILER_SUPPORTS_MSSSE3)
    target_sources(find_json_key PRIVATE src/matchers/matcher_teddy.cpp)
    set_source_files_properties(
        src/matchers/matcher_teddy.cpp
        PROPERTIES COMPILE_OPTIONS "-mssse3"
    )
    target_compile_definitions(find_json_key PUBLIC COMPILER_SUPPORTS_TEDDY=1)
else()
    message(STATUS "-mssse3 not supported!")
    target_compile_definitions(find_json_key PUBLIC COMPILER_SUPPORTS_TEDDY=0)
endif()

add_executable(findkey
    src/main.cpp
)

target_include_directories(findkey PRIVATE include src)
target_link_libraries(findkey PRIVATE find_json_key)
