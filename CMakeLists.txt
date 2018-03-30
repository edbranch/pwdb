cmake_minimum_required(VERSION 3.9)
project(pwdb VERSION "0.3.1.1")

include(GNUInstallDirs)

if(CMAKE_GENERATOR STREQUAL "Ninja")
    option(COMPILER_FORCE_COLOR "Force gcc diagnostic color output" ON)
endif()

if(COMPILER_FORCE_COLOR)
    add_compile_options("-fdiagnostics-color=always")
endif()

add_compile_options("-Wall")

enable_testing()

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory("lib")
add_subdirectory("include/pwdb")
add_subdirectory("src")
add_subdirectory("test")
