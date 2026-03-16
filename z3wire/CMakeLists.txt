# Concrete Bool wrapper (no Z3 dependency).
add_library(z3wire_bool INTERFACE)
add_library(z3wire::bool ALIAS z3wire_bool)

target_include_directories(z3wire_bool INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Concrete types (no Z3 dependency).
add_library(z3wire_int INTERFACE)
add_library(z3wire::int ALIAS z3wire_int)

target_include_directories(z3wire_int INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# SymBool wrapper.
add_library(z3wire_sym_bool INTERFACE)
add_library(z3wire::sym_bool ALIAS z3wire_sym_bool)

target_include_directories(z3wire_sym_bool INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(z3wire_sym_bool INTERFACE z3::libz3)

# Symbolic integer wrapper.
add_library(z3wire_sym_int INTERFACE)
add_library(z3wire::sym_int ALIAS z3wire_sym_int)

target_include_directories(z3wire_sym_int INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(z3wire_sym_int INTERFACE z3wire_sym_bool z3wire_int z3::libz3)

# Tests
include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(bool_test bool_test.cc)
    target_link_libraries(bool_test PRIVATE z3wire_bool GTest::gtest_main)
    add_test(NAME bool_test COMMAND bool_test)

    add_executable(int_test int_test.cc)
    target_link_libraries(int_test PRIVATE z3wire_int GTest::gtest_main)
    add_test(NAME int_test COMMAND int_test)

    add_executable(sym_bool_test sym_bool_test.cc)
    target_link_libraries(sym_bool_test PRIVATE z3wire_sym_bool GTest::gtest_main)
    add_test(NAME sym_bool_test COMMAND sym_bool_test)

    add_executable(sym_int_test sym_int_test.cc)
    target_link_libraries(sym_int_test PRIVATE z3wire_sym_int GTest::gtest_main)
    add_test(NAME sym_int_test COMMAND sym_int_test)
endif()
