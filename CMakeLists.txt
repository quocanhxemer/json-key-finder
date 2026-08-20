cmake_minimum_required(VERSION 3.16)
project(find_json_key LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(FIND_KEY_NATIVE "Enable -march=native" ON)

find_package(ZLIB QUIET)
find_path(XXHASH_INCLUDE_DIR NAMES xxhash.h)
find_library(XXHASH_LIBRARY
    NAMES
    xxhash
    libxxhash
    xxhash.so.0
    libxxhash.so.0
)

if(NOT UNIX
    OR NOT "cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES
    OR NOT ZLIB_FOUND
    OR NOT XXHASH_INCLUDE_DIR
    OR NOT XXHASH_LIBRARY)
    message(FATAL_ERROR
        "Missing build prerequisites. Requirements:\n"
        "  - CMake 3.16+ and a C++20 compiler\n"
        "  - POSIX-compatible system\n"
        "  - zlib development package (zlib1g-dev / zlib-devel)\n"
        "  - xxHash development package (libxxhash-dev / xxhash-devel)"
    )
endif()

add_library(find_json_key_warnings INTERFACE)
target_compile_options(find_json_key_warnings INTERFACE -Wall -Wextra -Wpedantic)

add_library(find_json_key_utils STATIC src/io/mmap_file.cpp)
target_include_directories(find_json_key_utils PUBLIC src)
target_link_libraries(find_json_key_utils PUBLIC find_json_key_warnings)

add_library(findkey_options STATIC src/core/findkey_options.cpp)
target_include_directories(findkey_options PUBLIC include src)
target_link_libraries(findkey_options PUBLIC find_json_key_warnings)

add_library(findkey_cli STATIC
    src/cli/args.cpp
    src/cli/reporting.cpp
)
target_include_directories(findkey_cli PUBLIC include src)
target_link_libraries(findkey_cli PUBLIC find_json_key_warnings findkey_options)

add_library(find_json_key STATIC
    src/core/findkey.cpp
    src/core/key_dfa.cpp
    src/core/prepared_keys.cpp

    src/teddy/compile.cpp
    src/teddy/configurations.cpp
    src/teddy/grouping.cpp
    src/teddy/suffix.cpp

    src/matchers/matcher_scalar.cpp
    src/matchers/matcher_teddy_baseline.cpp
)
target_include_directories(find_json_key
    PUBLIC include
    PRIVATE src
    ${XXHASH_INCLUDE_DIR}
)
target_link_libraries(find_json_key
    PUBLIC
    find_json_key_warnings
    ZLIB::ZLIB
    ${XXHASH_LIBRARY}
)

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

add_executable(findkey src/main.cpp)
target_include_directories(findkey PRIVATE include src)
target_link_libraries(findkey PRIVATE find_json_key find_json_key_utils findkey_cli)

add_library(find_key_generation STATIC tools/key_generation.cpp)
target_include_directories(find_key_generation PUBLIC tools)
target_link_libraries(find_key_generation PUBLIC find_json_key_warnings)

add_library(find_json_input STATIC tools/json_input.cpp)
target_include_directories(find_json_input PUBLIC src tools)
target_link_libraries(
    find_json_input
    PUBLIC
    find_json_key_utils
    find_key_generation
)

add_library(find_bench_tools STATIC
    tools/bench/bench_args.cpp
    tools/bench/bench_csv.cpp
    tools/bench/bench_runner.cpp
)
target_include_directories(find_bench_tools PUBLIC include src tools)
target_link_libraries(
    find_bench_tools
    PUBLIC
    find_json_key_warnings
    find_json_key
    findkey_options
    find_key_generation
)

add_executable(gen_keys tools/gen_keys.cpp)
target_include_directories(gen_keys PRIVATE include src tools)
target_link_libraries(gen_keys PRIVATE find_json_input find_key_generation)

add_executable(bench_matrix tools/bench_matrix.cpp)
target_include_directories(bench_matrix PRIVATE include src tools)
target_link_libraries(
    bench_matrix
    PRIVATE
    find_json_key
    find_json_input
    find_key_generation
    findkey_options
    find_bench_tools
)

add_executable(reverse_keys tools/reverse_keys.cpp)
target_include_directories(reverse_keys PRIVATE include src)
target_link_libraries(reverse_keys PRIVATE find_json_key_utils find_json_key_warnings)

if(BUILD_TESTING)
    find_package(GTest CONFIG QUIET)

    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(
        find_json_key_tests
        tests/capability_test.cpp
        tests/configurations_test.cpp
        tests/findkey_test.cpp
        tests/utils.cpp
    )
    target_include_directories(find_json_key_tests PRIVATE include src)
    target_compile_definitions(
        find_json_key_tests
        PRIVATE
        FINDKEY_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
    )
    target_link_libraries(
        find_json_key_tests
        PRIVATE
        find_json_key
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(find_json_key_tests)
endif()
