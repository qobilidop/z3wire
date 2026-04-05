// Prove that a gate-level ripple-carry adder matches intended semantics
// expressed in Z3Wire, for all 8-bit inputs.
//
// The hardware implementation builds an 8-bit adder from half adders and full
// adders using single-bit logic gates - the same structure a Verilog designer
// would write. The specification uses Z3Wire's type system to express the
// intended semantics in two lines. Z3 then proves they are equivalent.
//
// Run: ./dev.sh bazel run //examples:adder

#include <cstddef>
#include <iostream>
#include <utility>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

// -- Gate-level building blocks ---------------------------------------------

// Half adder: single-bit add without carry-in.
//   sum   = a ^ b
//   carry = a & b
std::pair<z3w::SymUInt<1>, z3w::SymUInt<1>> half_adder(z3w::SymUInt<1> a,
                                                       z3w::SymUInt<1> b) {
  return {a ^ b, a & b};
}

// Full adder: single-bit add with carry-in, built from two half adders.
//   {s1, c1} = half_adder(a, b)
//   {sum, c2} = half_adder(s1, cin)
//   cout      = c1 | c2
std::pair<z3w::SymUInt<1>, z3w::SymUInt<1>> full_adder(z3w::SymUInt<1> a,
                                                       z3w::SymUInt<1> b,
                                                       z3w::SymUInt<1> cin) {
  auto [s1, c1] = half_adder(a, b);
  auto [sum, c2] = half_adder(s1, cin);
  return {sum, c1 | c2};
}

// -- 8-bit ripple-carry adder -----------------------------------------------

// Extract bit I from a bit-vector as SymUInt<1>.
template <size_t I, size_t W, bool S>
z3w::SymUInt<1> bit(const z3w::SymBitVec<W, S>& val) {
  return z3w::extract<I, I>(val);
}

// Ripple-carry adder: chain 8 full adders, return {sum<8>, carry}.
std::pair<z3w::SymUInt<8>, z3w::SymUInt<1>> ripple_carry_adder(
    z3w::SymUInt<8> a, z3w::SymUInt<8> b) {
  z3w::SymUInt<1> carry = z3w::SymUInt<1>::Literal<0>(a.expr().ctx());

  auto [s0, c0] = full_adder(bit<0>(a), bit<0>(b), carry);
  auto [s1, c1] = full_adder(bit<1>(a), bit<1>(b), c0);
  auto [s2, c2] = full_adder(bit<2>(a), bit<2>(b), c1);
  auto [s3, c3] = full_adder(bit<3>(a), bit<3>(b), c2);
  auto [s4, c4] = full_adder(bit<4>(a), bit<4>(b), c3);
  auto [s5, c5] = full_adder(bit<5>(a), bit<5>(b), c4);
  auto [s6, c6] = full_adder(bit<6>(a), bit<6>(b), c5);
  auto [s7, c7] = full_adder(bit<7>(a), bit<7>(b), c6);

  auto sum = z3w::concat(s7, s6, s5, s4, s3, s2, s1, s0);
  return {sum, c7};
}

// -- Main: prove equivalence ------------------------------------------------

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");

  // Hardware implementation: gate-level ripple-carry adder.
  auto [hw_sum, hw_carry] = ripple_carry_adder(a, b);

  // Specification: Z3Wire bit-growth arithmetic.
  auto full = a + b;  // SymUInt<9>
  auto [spec_sum, value_preserved] = z3w::checked_cast<z3w::SymUInt<8>>(full);
  auto spec_carry = z3w::as_uint1(!value_preserved);

  // Prove equivalence: ask Z3 if the two implementations can ever disagree.
  solver.add((hw_sum != spec_sum || hw_carry != spec_carry).expr());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: ripple-carry adder matches intended semantics "
                 "for all 8-bit inputs.\n";
  } else {
    std::cout << "Bug found! Counter-example:\n";
    auto model = solver.get_model();
    std::cout << "  a = " << model.eval(a.expr()) << "\n";
    std::cout << "  b = " << model.eval(b.expr()) << "\n";
  }

  return 0;
}
