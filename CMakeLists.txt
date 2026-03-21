cmake_minimum_required(VERSION 3.16)
project(z3wire LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Locate the pre-built Z3 installed in the dev container (see Dockerfile).
find_path(Z3_INCLUDE_DIR NAMES z3++.h REQUIRED)
find_library(Z3_LIBRARY NAMES z3 REQUIRED)
add_library(z3::libz3 IMPORTED INTERFACE)
set_target_properties(z3::libz3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Z3_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${Z3_LIBRARY}"
)

find_package(GTest CONFIG REQUIRED)

include(CTest)

add_subdirectory(z3wire)
add_subdirectory(examples)

option(Z3WIRE_BUILD_WEAVE "Build the Weave code generator" OFF)
if(Z3WIRE_BUILD_WEAVE)
    add_subdirectory(z3wire_weave)
endif()
