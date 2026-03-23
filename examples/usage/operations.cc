// Snippets demonstrating Z3Wire's operations.
//
// Shows logical, bitwise, comparison, arithmetic, shifting, rotation, bit
// manipulation, and conditional selection operations.
//
// Run: ./dev.sh bazel run //examples/usage:operations

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"
#include "z3wire/sym_bool.h"

// --- Logical ---

void demo_logical() {
  z3::context ctx;
  z3w::SymBool a(ctx, "a");
  z3w::SymBool b(ctx, "b");

  z3w::SymBool r1 = !a;
  z3w::SymBool r2 = a && b;
  z3w::SymBool r3 = a || b;
  z3w::SymBool r4 = a ^ b;
}

// --- Bitwise ---

void demo_bitwise() {
  z3::context ctx;
  z3w::SymUInt<8> u8(ctx, "u8");
  z3w::SymSInt<8> s8(ctx, "s8");

  z3w::SymUInt<8> r1 = ~u8;
  z3w::SymSInt<8> r2 = ~s8;

  z3w::SymUInt<8> r3 = u8 & u8;
  z3w::SymSInt<8> r4 = s8 & s8;

  z3w::SymUInt<8> r5 = u8 | u8;
  z3w::SymSInt<8> r6 = s8 | s8;

  z3w::SymUInt<8> r7 = u8 ^ u8;
  z3w::SymSInt<8> r8 = s8 ^ s8;
}

// --- Comparison ---

void demo_boolean_comparison() {
  z3::context ctx;
  z3w::SymBool a(ctx, "a");
  z3w::SymBool b(ctx, "b");

  z3w::SymBool eq = (a == b);
  z3w::SymBool ne = (a != b);
}

void demo_integer_comparison() {
  z3::context ctx;
  z3w::SymUInt<7> a(ctx, "a");
  z3w::SymSInt<9> b(ctx, "b");

  z3w::SymBool eq = (a == b);
  z3w::SymBool ne = (a != b);
  z3w::SymBool gt = (a > b);
  z3w::SymBool ge = (a >= b);
  z3w::SymBool lt = (a < b);
  z3w::SymBool le = (a <= b);
}

// --- Arithmetic ---

void demo_addition() {
  z3::context ctx;
  z3w::SymUInt<7> u7(ctx, "u7");
  z3w::SymUInt<9> u9(ctx, "u9");
  z3w::SymSInt<7> s7(ctx, "s7");
  z3w::SymSInt<9> s9(ctx, "s9");

  z3w::SymUInt<10> r1 = u7 + u9;
  z3w::SymSInt<10> r2 = s7 + s9;

  z3w::SymSInt<11> r3 = u9 + s7;
  z3w::SymSInt<10> r4 = u7 + s9;

  z3w::SymSInt<11> r5 = s7 + u9;
  z3w::SymSInt<10> r6 = s9 + u7;
}

void demo_subtraction() {
  z3::context ctx;
  z3w::SymUInt<7> u7(ctx, "u7");
  z3w::SymUInt<9> u9(ctx, "u9");
  z3w::SymSInt<7> s7(ctx, "s7");
  z3w::SymSInt<9> s9(ctx, "s9");

  z3w::SymSInt<10> r1 = u7 - u9;
  z3w::SymSInt<10> r2 = s7 - s9;

  z3w::SymSInt<11> r3 = u9 - s7;
  z3w::SymSInt<10> r4 = u7 - s9;

  z3w::SymSInt<11> r5 = s7 - u9;
  z3w::SymSInt<10> r6 = s9 - u7;
}

void demo_negate() {
  z3::context ctx;
  z3w::SymUInt<8> u8(ctx, "u8");
  z3w::SymSInt<8> s8(ctx, "s8");

  z3w::SymSInt<9> r1 = -u8;
  z3w::SymSInt<9> r2 = -s8;
}

// --- Shifting ---

void demo_shl() {
  z3::context ctx;
  z3w::SymUInt<8> u8(ctx, "u8");
  z3w::SymSInt<8> s8(ctx, "s8");
  z3w::SymUInt<3> n(ctx, "n");

  z3w::SymUInt<11> r1 = z3w::shl<3>(u8);
  z3w::SymUInt<15> r2 = z3w::shl(u8, n);
  z3w::SymUInt<11> r3 = z3w::shl<3>(s8);
  z3w::SymUInt<15> r4 = z3w::shl(s8, n);
}

void demo_shr() {
  z3::context ctx;
  z3w::SymUInt<8> u8(ctx, "u8");
  z3w::SymSInt<8> s8(ctx, "s8");
  z3w::SymUInt<3> n(ctx, "n");

  z3w::SymUInt<8> r1 = z3w::shr<3>(u8);
  z3w::SymUInt<8> r2 = z3w::shr(u8, n);
  z3w::SymSInt<8> r3 = z3w::shr<3>(s8);
  z3w::SymSInt<8> r4 = z3w::shr(s8, n);
}

// --- Rotation ---

void demo_rotl() {
  z3::context ctx;
  z3w::SymUInt<8> u8(ctx, "u8");
  z3w::SymSInt<8> s8(ctx, "s8");
  z3w::SymUInt<3> n(ctx, "n");

  z3w::SymUInt<8> r1 = z3w::rotl<3>(u8);
  z3w::SymUInt<8> r2 = z3w::rotl(u8, n);
  z3w::SymSInt<8> r3 = z3w::rotl<3>(s8);
  z3w::SymSInt<8> r4 = z3w::rotl(s8, n);
}

void demo_rotr() {
  z3::context ctx;
  z3w::SymUInt<8> u8(ctx, "u8");
  z3w::SymSInt<8> s8(ctx, "s8");
  z3w::SymUInt<3> n(ctx, "n");

  z3w::SymUInt<8> r1 = z3w::rotr<3>(u8);
  z3w::SymUInt<8> r2 = z3w::rotr(u8, n);
  z3w::SymSInt<8> r3 = z3w::rotr<3>(s8);
  z3w::SymSInt<8> r4 = z3w::rotr(s8, n);
}

// --- Bit manipulation ---

void demo_static_extraction() {
  z3::context ctx;
  z3w::SymUInt<32> src(ctx, "src");

  z3w::SymUInt<8> hi = z3w::extract<31, 24>(src);
  z3w::SymUInt<4> lo = z3w::extract<3, 0>(src);
  z3w::SymUInt<1> bit5 = z3w::extract<5, 5>(src);
}

void demo_symbolic_extraction() {
  z3::context ctx;
  z3w::SymUInt<32> src(ctx, "src");
  z3w::SymUInt<5> offset(ctx, "offset");

  z3w::SymUInt<4> nibble = z3w::extract<4>(src, offset);
  z3w::SymUInt<8> byte = z3w::extract<8>(src, offset);
}

void demo_concat() {
  z3::context ctx;
  z3w::SymUInt<16> hi(ctx, "hi");
  z3w::SymUInt<16> lo(ctx, "lo");

  z3w::SymUInt<32> r1 = z3w::concat(hi, lo);

  z3w::SymUInt<4> a(ctx, "a");
  z3w::SymUInt<4> b(ctx, "b");
  z3w::SymSInt<8> c(ctx, "c");
  z3w::SymUInt<16> r2 = z3w::concat(a, b, c);
}

// --- Conditional selection ---

void demo_ite() {
  z3::context ctx;
  z3w::SymBool sel(ctx, "sel");
  z3w::SymUInt<8> a2(ctx, "a2");
  z3w::SymUInt<8> b2(ctx, "b2");
  z3w::SymSInt<8> a3(ctx, "a3");
  z3w::SymSInt<8> b3(ctx, "b3");

  z3w::SymUInt<8> r2 = z3w::ite(sel, a2, b2);
  z3w::SymSInt<8> r3 = z3w::ite(sel, a3, b3);
}

int main() {
  demo_logical();
  demo_bitwise();
  demo_boolean_comparison();
  demo_integer_comparison();
  demo_addition();
  demo_subtraction();
  demo_negate();
  demo_shl();
  demo_shr();
  demo_rotl();
  demo_rotr();
  demo_static_extraction();
  demo_symbolic_extraction();
  demo_concat();
  demo_ite();
  return 0;
}
