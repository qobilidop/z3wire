# Concrete Bool wrapper (no Z3 dependency).
add_library(z3wire_bool INTERFACE)
add_library(z3wire::bool ALIAS z3wire_bool)

target_include_directories(z3wire_bool INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Concrete types (no Z3 dependency).
add_library(z3wire_bit_vec INTERFACE)
add_library(z3wire::bit_vec ALIAS z3wire_bit_vec)

target_include_directories(z3wire_bit_vec INTERFACE
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

# Symbolic bit-vector wrapper.
add_library(z3wire_sym_bit_vec INTERFACE)
add_library(z3wire::sym_bit_vec ALIAS z3wire_sym_bit_vec)

target_include_directories(z3wire_sym_bit_vec INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(z3wire_sym_bit_vec INTERFACE z3wire_sym_bool z3wire_bit_vec z3::libz3)

# Tests
if(BUILD_TESTING)
    add_executable(bool_test bool_test.cc)
    target_link_libraries(bool_test PRIVATE z3wire_bool GTest::gtest_main)
    add_test(NAME bool_test COMMAND bool_test)

    add_executable(bit_vec_test bit_vec_test.cc)
    target_link_libraries(bit_vec_test PRIVATE z3wire_bit_vec GTest::gtest_main)
    add_test(NAME bit_vec_test COMMAND bit_vec_test)

    add_executable(sym_bool_test sym_bool_test.cc)
    target_link_libraries(sym_bool_test PRIVATE z3wire_sym_bool GTest::gtest_main)
    add_test(NAME sym_bool_test COMMAND sym_bool_test)

    add_executable(sym_bit_vec_test sym_bit_vec_test.cc)
    target_link_libraries(sym_bit_vec_test PRIVATE z3wire_sym_bit_vec GTest::gtest_main)
    add_test(NAME sym_bit_vec_test COMMAND sym_bit_vec_test)
endif()

add_subdirectory(weave)
