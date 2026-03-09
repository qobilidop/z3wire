add_executable(safe_adder safe_adder.cc)
target_link_libraries(safe_adder PRIVATE z3wire_bitvec)

add_executable(alu alu.cc)
target_link_libraries(alu PRIVATE z3wire_bitvec)

add_executable(bit_manipulation bit_manipulation.cc)
target_link_libraries(bit_manipulation PRIVATE z3wire_bitvec)
