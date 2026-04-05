add_executable(adder adder.cc)
target_link_libraries(adder PRIVATE z3wire_sym_bit_vec)

add_executable(barrel_shifter barrel_shifter.cc)
target_link_libraries(barrel_shifter PRIVATE z3wire_sym_bit_vec)

add_executable(alu alu.cc)
target_link_libraries(alu PRIVATE z3wire_sym_bit_vec)

add_executable(bit_manipulation bit_manipulation.cc)
target_link_libraries(bit_manipulation PRIVATE z3wire_sym_bit_vec)

add_executable(packet_gen packet_gen.cc)
target_link_libraries(packet_gen PRIVATE z3wire_sym_bit_vec)

add_executable(multiplier multiplier.cc)
target_link_libraries(multiplier PRIVATE z3wire_sym_bit_vec)

add_executable(midpoint_overflow midpoint_overflow.cc)
target_link_libraries(midpoint_overflow PRIVATE z3wire_sym_bit_vec)

# Usage examples (also embedded in docs).
add_executable(usage_types usage/types.cc)
target_link_libraries(usage_types PRIVATE z3wire_sym_bit_vec)

add_executable(usage_operations usage/operations.cc)
target_link_libraries(usage_operations PRIVATE z3wire_sym_bit_vec)

add_executable(usage_type_conversions usage/type_conversions.cc)
target_link_libraries(usage_type_conversions PRIVATE z3wire_sym_bit_vec)
