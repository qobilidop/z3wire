# Concrete types (no Z3 dependency).
add_library(z3wire_int INTERFACE)
add_library(z3wire::int ALIAS z3wire_int)

target_include_directories(z3wire_int INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Bool wrapper.
add_library(z3wire_bool INTERFACE)
add_library(z3wire::bool ALIAS z3wire_bool)

target_include_directories(z3wire_bool INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(z3wire_bool INTERFACE z3::libz3)

# Bit-vector wrapper.
add_library(z3wire_bitvec INTERFACE)
add_library(z3wire::bitvec ALIAS z3wire_bitvec)

target_include_directories(z3wire_bitvec INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(z3wire_bitvec INTERFACE z3wire_bool z3wire_int z3::libz3)

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

    add_executable(int_test int_test.cc)
    target_link_libraries(int_test PRIVATE z3wire_int GTest::gtest_main)
    add_test(NAME int_test COMMAND int_test)

    add_executable(bool_test bool_test.cc)
    target_link_libraries(bool_test PRIVATE z3wire_bool GTest::gtest_main)
    add_test(NAME bool_test COMMAND bool_test)

    add_executable(bitvec_test bitvec_test.cc)
    target_link_libraries(bitvec_test PRIVATE z3wire_bitvec GTest::gtest_main)
    add_test(NAME bitvec_test COMMAND bitvec_test)
endif()
