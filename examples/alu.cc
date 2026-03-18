// Model a simple 4-operation ALU and verify its properties.
//
// The ALU takes two 8-bit inputs and a 2-bit opcode, producing an 8-bit result.
// We verify that AND never exceeds its inputs and find an example of ADD
// overflow.
//
// Run: ./dev.sh bazel run //examples:alu

#include <iostream>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");
  z3w::SymUInt<2> opcode(ctx, "opcode");

  // ALU operations (all truncated to 8-bit hardware width).
  auto add_result = z3w::unsafe_cast<z3w::SymUInt<8>>(a + b);
  auto sub_result = z3w::unsafe_cast<z3w::SymUInt<8>>(a - b);
  auto and_result = a & b;
  auto or_result = a | b;

  // Mux the result based on opcode.
  auto op0 = z3w::SymUInt<2>::Literal<0>(ctx);
  auto op1 = z3w::SymUInt<2>::Literal<1>(ctx);
  auto op2 = z3w::SymUInt<2>::Literal<2>(ctx);

  auto result =
      z3w::ite(opcode == op0, add_result,
               z3w::ite(opcode == op1, sub_result,
                        z3w::ite(opcode == op2, and_result, or_result)));

  // Property: for AND operation, result is always <= both inputs.
  solver.push();
  solver.add(opcode.expr() == op2.expr());
  solver.add((result > a).expr() || (result > b).expr());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: AND result is always <= both inputs.\n";
  } else {
    std::cout << "Bug found!\n";
  }
  solver.pop();

  // Property: for ADD, find inputs where overflow occurs.
  auto sum9 = a + b;
  auto [_, value_preserved] = z3w::checked_cast<z3w::SymUInt<8>>(sum9);

  solver.push();
  solver.add(!value_preserved.expr());

  if (solver.check() == z3::sat) {
    auto model = solver.get_model();
    std::cout << "ADD overflow example: a=" << model.eval(a.expr())
              << ", b=" << model.eval(b.expr()) << "\n";
  }
  solver.pop();

  return 0;
}
