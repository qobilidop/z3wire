cmake_minimum_required(VERSION 3.16)
project(z3wire LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Z3 via CMake config (source builds) or pkg-config (system packages).
find_package(Z3 QUIET)
if(NOT Z3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Z3 REQUIRED IMPORTED_TARGET z3)
    add_library(z3::libz3 ALIAS PkgConfig::Z3)
endif()

include(CTest)

add_subdirectory(z3wire)
add_subdirectory(examples)
