// Verify that an 8-bit adder with overflow detection is correct.
//
// A common hardware pattern: add two 8-bit values, produce an 8-bit result and
// a carry flag. Z3Wire's bit-growth arithmetic lets us verify this without
// manually managing widths.

#include <iostream>

#include <z3++.h>

#include "z3wire/bitvec.h"

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::Ubv<8> a(ctx, "a");
  z3w::Ubv<8> b(ctx, "b");

  // Bit-growth addition: result is 9 bits, no overflow possible.
  auto sum9 = a + b;  // z3w::Ubv<9>

  // Hardware truncation: take the low 8 bits.
  auto result = z3w::cast<z3w::Ubv<8>>(sum9);

  // Carry flag: the 9th bit.
  auto carry = z3w::to_bool(z3w::extract<8, 8>(sum9));

  // Verify: if no carry, the truncated result equals the full sum.
  auto [checked_result, overflowed] =
      z3w::checked_cast<z3w::Ubv<8>>(sum9);
  solver.add(carry.raw() != overflowed.raw());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: carry flag matches overflow detection.\n";
  } else {
    std::cout << "Bug found! Counter-example:\n";
    auto model = solver.get_model();
    std::cout << "  a = " << model.eval(a.raw()) << "\n";
    std::cout << "  b = " << model.eval(b.raw()) << "\n";
  }

  return 0;
}
