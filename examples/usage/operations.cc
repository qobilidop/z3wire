// Snippets demonstrating Z3Wire's operations.
//
// Shows logical, bitwise, comparison, arithmetic, shifting, bit manipulation,
// and mux operations.
//
// Run: ./dev.sh bazel run //examples/usage:operations

#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/sym_bit_vec.h"
#include "z3wire/sym_bool.h"

// --- Logical ---

void demo_logical() {
  z3::context ctx;
  z3w::SymBool a(ctx, "a");
  z3w::SymBool b(ctx, "b");

  z3w::SymBool c = a && b;  // AND
  z3w::SymBool d = a || b;  // OR
  z3w::SymBool e = a ^ b;   // XOR
  z3w::SymBool f = !a;      // NOT
}

// --- Bitwise ---

void demo_bitwise() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");

  auto c = a & b;  // AND
  auto d = a | b;  // OR
  auto e = a ^ b;  // XOR
  auto f = ~a;     // NOT
}

// --- Comparison ---

void demo_equality() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");
  z3w::SymBool eq = (a == b);
  z3w::SymBool ne = (a != b);

  // Mixed widths:
  z3w::SymUInt<16> c(ctx, "c");
  z3w::SymBool eq2 = (a == c);  // a is zero-extended to 16 bits first
}

void demo_ordered_comparison() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");
  z3w::SymBool less = (a < b);  // Unsigned comparison

  z3w::SymSInt<8> x(ctx, "x");
  z3w::SymSInt<8> y(ctx, "y");
  z3w::SymBool less_s = (x < y);  // Signed comparison

  // Mixed signedness:
  z3w::SymBool less_m = (a < x);  // Common type: SymSInt<9>
}

// --- Arithmetic ---

void demo_addition() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");
  auto sum = a + b;      // SymUInt<9>
  auto total = sum + a;  // SymUInt<10>

  // Mixed widths:
  z3w::SymUInt<16> c(ctx, "c");
  auto sum2 = a + c;  // SymUInt<17>, a is zero-extended first
}

void demo_subtraction() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");
  auto diff = a - b;  // SymSInt<9>
}

void demo_negate() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  auto neg = -a;  // SymSInt<9>

  // Truncate to fixed width:
  z3w::SymUInt<8> b(ctx, "b");
  auto reg = z3w::unsafe_cast<z3w::SymUInt<8>>(a + b);  // Truncate to 8 bits
}

// --- Shifting ---

void demo_shl() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");

  // Constant shift
  auto r1 = z3w::shl<3>(a);  // SymUInt<11>

  // Symbolic shift
  z3w::SymUInt<3> n(ctx, "n");  // Can shift by 0..7
  auto r2 = z3w::shl(a, n);     // SymUInt<15> (8 + 2^3 - 1)
}

void demo_shr() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<3> n(ctx, "n");

  // Constant shift
  auto r1 = z3w::shr<3>(a);  // SymUInt<8>

  // Symbolic shift
  auto r2 = z3w::shr(a, n);  // SymUInt<8>

  // Signed: sign bit preserved
  z3w::SymSInt<8> x(ctx, "x");
  auto r3 = z3w::shr(x, n);  // SymSInt<8>, sign bit preserved
}

// --- Bit manipulation ---

void demo_bit_extraction() {
  z3::context ctx;
  z3w::SymUInt<32> x(ctx, "x");

  // Single-bit extraction
  auto sign = z3w::bit<31>(x);  // SymUInt<1>
  auto lsb = z3w::bit<0>(x);    // SymUInt<1>

  // Static extract
  auto hi = z3w::extract<31, 24>(x);  // SymUInt<8>
  auto lo = z3w::extract<3, 0>(x);    // SymUInt<4>
  auto bit5 = z3w::extract<5, 5>(x);  // SymUInt<1>

  // Symbolic-offset extract
  z3w::SymUInt<5> offset(ctx, "offset");
  auto nibble = z3w::extract<4>(x, offset);  // SymUInt<4>
  auto byte = z3w::extract<8>(x, offset);    // SymUInt<8>
}

void demo_concat() {
  z3::context ctx;
  z3w::SymUInt<16> hi(ctx, "hi");
  z3w::SymUInt<16> lo(ctx, "lo");
  auto full = z3w::concat(hi, lo);  // SymUInt<32>

  // Variadic:
  z3w::SymUInt<4> a(ctx, "a");
  z3w::SymUInt<4> b(ctx, "b");
  z3w::SymUInt<8> c(ctx, "c");
  auto packed = z3w::concat(a, b, c);  // SymUInt<16>
}

// --- Mux ---

void demo_mux() {
  z3::context ctx;
  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymUInt<8> b(ctx, "b");
  z3w::SymBool sel(ctx, "sel");

  auto result = z3w::ite(sel, a, b);  // SymUInt<8>

  // With concrete values:
  z3w::SymBool cond(ctx, "cond");
  auto x = z3w::UInt<8>::Literal<42>();
  auto y = z3w::UInt<8>::Literal<99>();
  auto result2 = z3w::ite(cond, x, y);  // Promotes both to symbolic
}

int main() {
  demo_logical();
  demo_bitwise();
  demo_equality();
  demo_ordered_comparison();
  demo_addition();
  demo_subtraction();
  demo_negate();
  demo_shl();
  demo_shr();
  demo_bit_extraction();
  demo_concat();
  demo_mux();
  return 0;
}
