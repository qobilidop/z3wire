// Prove that a gate-level barrel shifter matches intended semantics expressed
// in Z3Wire, for all 8-bit inputs and 3-bit shift amounts.
//
// The hardware implementation builds a barrel shifter from three layers of
// 2:1 multiplexers - one layer per bit of the shift amount. Each layer
// conditionally shifts by a power of two. The specification uses Z3Wire's
// type system to express the intended semantics in one line. Z3 then proves
// they are equivalent.
//
// Run: ./dev.sh bazel run //examples:barrel_shifter

#include <cstddef>
#include <iostream>

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"

// -- Gate-level building blocks ---------------------------------------------

// 2:1 mux on a single bit: sel=1 selects a, sel=0 selects b.
//   out = (sel & a) | (~sel & b)
z3w::SymUInt<1> mux(z3w::SymUInt<1> sel, z3w::SymUInt<1> a, z3w::SymUInt<1> b) {
  return (sel & a) | (~sel & b);
}

// Extract bit I from a bit-vector as SymUInt<1>.
template <size_t I, size_t W, bool S>
z3w::SymUInt<1> bit(const z3w::SymBitVec<W, S>& val) {
  return z3w::extract<1, I>(val);
}

// -- 8-bit barrel shifter (left shift) --------------------------------------

// Three layers of muxes, one per bit of the 3-bit shift amount.
// Layer k: if amt[k]=1, shift left by 2^k; else pass through.
// Bits shifted in from the right are zero.
z3w::SymUInt<8> barrel_shift_left(z3w::SymUInt<8> data, z3w::SymUInt<3> amt) {
  auto zero = z3w::SymUInt<1>::Literal<0>(data.expr().ctx());

  // Layer 0: shift by 1 if amt[0]=1.
  auto s0 = bit<0>(amt);
  auto r0 = z3w::concat(
      mux(s0, bit<6>(data), bit<7>(data)), mux(s0, bit<5>(data), bit<6>(data)),
      mux(s0, bit<4>(data), bit<5>(data)), mux(s0, bit<3>(data), bit<4>(data)),
      mux(s0, bit<2>(data), bit<3>(data)), mux(s0, bit<1>(data), bit<2>(data)),
      mux(s0, bit<0>(data), bit<1>(data)), mux(s0, zero, bit<0>(data)));

  // Layer 1: shift by 2 if amt[1]=1.
  auto s1 = bit<1>(amt);
  auto r1 = z3w::concat(
      mux(s1, bit<5>(r0), bit<7>(r0)), mux(s1, bit<4>(r0), bit<6>(r0)),
      mux(s1, bit<3>(r0), bit<5>(r0)), mux(s1, bit<2>(r0), bit<4>(r0)),
      mux(s1, bit<1>(r0), bit<3>(r0)), mux(s1, bit<0>(r0), bit<2>(r0)),
      mux(s1, zero, bit<1>(r0)), mux(s1, zero, bit<0>(r0)));

  // Layer 2: shift by 4 if amt[2]=1.
  auto s2 = bit<2>(amt);
  auto r2 = z3w::concat(
      mux(s2, bit<3>(r1), bit<7>(r1)), mux(s2, bit<2>(r1), bit<6>(r1)),
      mux(s2, bit<1>(r1), bit<5>(r1)), mux(s2, bit<0>(r1), bit<4>(r1)),
      mux(s2, zero, bit<3>(r1)), mux(s2, zero, bit<2>(r1)),
      mux(s2, zero, bit<1>(r1)), mux(s2, zero, bit<0>(r1)));

  return r2;
}

// -- Main: prove equivalence ------------------------------------------------

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::SymUInt<8> data(ctx, "data");
  z3w::SymUInt<3> amt(ctx, "amt");

  // Hardware implementation: gate-level barrel shifter.
  auto hw_result = barrel_shift_left(data, amt);

  // Specification: Z3Wire auto-widening left shift, truncated to 8 bits.
  auto [spec_result, _] =
      z3w::checked_cast<z3w::SymUInt<8>>(z3w::shl(data, amt));

  // Prove equivalence: ask Z3 if the two implementations can ever disagree.
  solver.add((hw_result != spec_result).expr());

  if (solver.check() == z3::unsat) {
    std::cout << "Verified: barrel shifter matches intended semantics for all "
                 "8-bit data and 3-bit shift amounts.\n";
  } else {
    std::cout << "Bug found! Counter-example:\n";
    auto model = solver.get_model();
    std::cout << "  data = " << model.eval(data.expr()) << "\n";
    std::cout << "  amt  = " << model.eval(amt.expr()) << "\n";
  }

  return 0;
}
