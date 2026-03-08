add_library(z3wire INTERFACE)
add_library(z3wire::z3wire ALIAS z3wire)

target_include_directories(z3wire INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(z3wire INTERFACE z3::libz3)

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
    target_link_libraries(bool_test PRIVATE z3wire GTest::gtest_main)
    add_test(NAME bool_test COMMAND bool_test)

    add_executable(bitvec_test bitvec_test.cc)
    target_link_libraries(bitvec_test PRIVATE z3wire GTest::gtest_main)
    add_test(NAME bitvec_test COMMAND bitvec_test)
endif()
