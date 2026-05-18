// Prove that a gate-level array multiplier matches intended semantics expressed
// in Z3Wire, for all 4-bit inputs.
//
// The hardware implementation builds a 4x4 array multiplier from AND gates and
// full/half adders - the same structure found in digital design textbooks. Each
// row generates a partial product (b[i] AND a), and rows are summed with
// carry-propagate adders. The specification uses Z3Wire's type system to
// express the intended semantics in one line. Z3 then proves they are
// equivalent.
//
// Run: ./dev.sh bazel run //examples:multiplier

#include <cstddef>
#include <iostream>
#include <utility>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

// -- Gate-level building blocks ---------------------------------------------

// Half adder: single-bit add without carry-in.
std::pair<z3w::SymUInt<1>, z3w::SymUInt<1>> half_adder(z3w::SymUInt<1> a,
                                                       z3w::SymUInt<1> b) {
  return {a ^ b, a & b};
}

// Full adder: single-bit add with carry-in, built from two half adders.
std::pair<z3w::SymUInt<1>, z3w::SymUInt<1>> full_adder(z3w::SymUInt<1> a,
                                                       z3w::SymUInt<1> b,
                                                       z3w::SymUInt<1> cin) {
  auto [s1, c1] = half_adder(a, b);
  auto [sum, c2] = half_adder(s1, cin);
  return {sum, c1 | c2};
}

// Extract bit I from a bit-vector as SymUInt<1>.
template <size_t I, size_t W, bool S>
z3w::SymUInt<1> bit(const z3w::SymBitVec<W, S>& val) {
  return z3w::extract<1, I>(val);
}

// -- 4x4 array multiplier --------------------------------------------------

// Partial product: b[i] AND each bit of a.
//   pp[j] = a[j] & b[i]
//
// Array layout (each pp_ij = a[j] & b[i]):
//
//                    pp_03  pp_02  pp_01  pp_00     row 0 (b[0] & a)
//             pp_13  pp_12  pp_11  pp_10            row 1 (b[1] & a)
//      pp_23  pp_22  pp_21  pp_20                   row 2 (b[2] & a)
//  +   pp_33  pp_32  pp_31  pp_30                   row 3 (b[3] & a)
//  ─────────────────────────────────────────
//  p7     p6     p5     p4     p3  p2  p1  p0       product
//
// Rows are summed using half and full adders, propagating carries upward.

z3w::SymUInt<8> array_multiply(z3w::SymUInt<4> a, z3w::SymUInt<4> b) {
  auto a0 = bit<0>(a);
  auto a1 = bit<1>(a);
  auto a2 = bit<2>(a);
  auto a3 = bit<3>(a);
  auto b0 = bit<0>(b);
  auto b1 = bit<1>(b);
  auto b2 = bit<2>(b);
  auto b3 = bit<3>(b);

  // Partial products: pp_ij = b[i] & a[j].
  auto pp00 = b0 & a0;
  auto pp01 = b0 & a1;
  auto pp02 = b0 & a2;
  auto pp03 = b0 & a3;
  auto pp10 = b1 & a0;
  auto pp11 = b1 & a1;
  auto pp12 = b1 & a2;
  auto pp13 = b1 & a3;
  auto pp20 = b2 & a0;
  auto pp21 = b2 & a1;
  auto pp22 = b2 & a2;
  auto pp23 = b2 & a3;
  auto pp30 = b3 & a0;
  auto pp31 = b3 & a1;
  auto pp32 = b3 & a2;
  auto pp33 = b3 & a3;

  // Add row 0 (shifted) + row 1:
  //   pp03  pp02  pp01
  // + pp13  pp12  pp11  pp10
  auto [p1, c1] = half_adder(pp01, pp10);
  auto [s2a, c2a] = full_adder(pp02, pp11, c1);
  auto [s3a, c3a] = full_adder(pp03, pp12, c2a);
  auto [s4a, c4a] = half_adder(pp13, c3a);

  // Add previous sum + row 2:
  //   c4a   s3a   s2a
  // + pp23  pp22  pp21  pp20
  auto [p2, c2b] = half_adder(s2a, pp20);
  auto [s3b, c3b] = full_adder(s3a, pp21, c2b);
  auto [s4b, c4b] = full_adder(s4a, pp22, c3b);
  auto [s5a, c5a] = full_adder(c4a, pp23, c4b);

  // Add previous sum + row 3:
  //   c5a   s4b   s3b
  // + pp33  pp32  pp31  pp30
  auto [p3, c3c] = half_adder(s3b, pp30);
  auto [p4, c4c] = full_adder(s4b, pp31, c3c);
  auto [p5, c5b] = full_adder(s5a, pp32, c4c);
  auto [p6, p7] = full_adder(c5a, pp33, c5b);

  return z3w::concat(p7, p6, p5, p4, p3, p2, p1, pp00);
}

// -- Main: prove equivalence ------------------------------------------------

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::SymUInt<4> a(ctx, "a");
  z3w::SymUInt<4> b(ctx, "b");

  // Hardware implementation: gate-level array multiplier.
  auto hw_result = array_multiply(a, b);

  // Specification: Z3Wire bit-growth multiplication.
  auto spec_result = a * b;  // SymUInt<8>

  // Prove equivalence: ask Z3 if the two implementations can ever disagree.
  solver.add((hw_result != spec_result).expr());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: array multiplier matches intended semantics for "
                 "all 4-bit inputs.\n";
  } else {
    std::cout << "Bug found! Counter-example:\n";
    auto model = solver.get_model();
    std::cout << "  a = " << model.eval(a.expr()) << "\n";
    std::cout << "  b = " << model.eval(b.expr()) << "\n";
  }

  return 0;
}
