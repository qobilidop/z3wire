// Prove that the classic binary search midpoint formula (a + b) >> 1 can
// overflow, and that the bit-hack (a & b) + ((a ^ b) >> 1) is correct.
//
// This bug lurked in Java's Arrays.binarySearch for 9 years before Joshua
// Bloch discovered it in 2006. Z3Wire's bit-growth arithmetic makes the
// intended correct semantics natural to express and verify.

#include <iostream>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::SymUInt<32> a(ctx, "a");
  z3w::SymUInt<32> b(ctx, "b");

  // Result types are derived at compile time. Written out explicitly for
  // clarity.

  // Buggy: ((uint32_t) (a + b)) >> 1
  // Truncating sum wraps, then shift gives wrong answer.
  z3w::SymUInt<32> buggy =
      z3w::shr<1>(z3w::unsafe_cast<z3w::SymUInt<32>>(a + b));

  // Bit-hack fix: (uint32_t) ((a & b) + ((a ^ b) >> 1))
  // Magical! We shall prove it's correct.
  z3w::SymUInt<32> hack =
      z3w::unsafe_cast<z3w::SymUInt<32>>((a & b) + z3w::shr<1>(a ^ b));

  // Z3Wire: (a + b) >> 1
  // Expresses the intended correct semantics naturally.
  z3w::SymUInt<33> correct = z3w::shr<1>(a + b);

  // Prove the buggy version can produce wrong results.
  solver.push();
  solver.add((buggy != correct).expr());

  if (solver.check() == z3::sat) {
    auto model = solver.get_model();
    std::cout << "Buggy midpoint overflows! Counter-example:\n";
    std::cout << "  a = " << model.eval(a.expr()) << "\n";
    std::cout << "  b = " << model.eval(b.expr()) << "\n";
  } else {
    std::cout << "No overflow found (unexpected).\n";
  }

  solver.pop();

  // Prove the bit-hack always matches the correct result.
  solver.push();

  // Z3Wire comparison is mathematical: SymUInt<32> and SymUInt<33> can be
  // compared directly, checking whether their values are always equal.
  solver.add((hack != correct).expr());

  if (solver.check() == z3::unsat) {
    std::cout << "Bit-hack verified correct for all inputs.\n";
  } else {
    std::cout << "Bit-hack has a bug (unexpected).\n";
  }

  solver.pop();

  return 0;
}
