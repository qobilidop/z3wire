#include "z3wire/bitvec.h"

#include <gtest/gtest.h>
#include <z3++.h>

namespace z3w {
namespace {

class BitVecTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

// --- Construction ---

TEST_F(BitVecTest, SymbolicVariable) {
  Ubv<8> a(ctx_, "a");
  EXPECT_EQ(a.raw().get_sort().bv_size(), 8);
}

TEST_F(BitVecTest, Literal) {
  auto val = Ubv<8>::Literal<255>(ctx_);
  z3::solver s(ctx_);
  s.add(val.raw() == ctx_.bv_val(255, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(BitVecTest, LiteralZero) {
  auto val = Ubv<8>::Literal<0>(ctx_);
  z3::solver s(ctx_);
  s.add(val.raw() == ctx_.bv_val(0, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

// --- Bitwise ---

TEST_F(BitVecTest, BitwiseAnd) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Ubv<8> c = a & b;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xF0, 8));
  s.add(b.raw() == ctx_.bv_val(0x3C, 8));
  s.add(c.raw() != ctx_.bv_val(0x30, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, BitwiseOr) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Ubv<8> c = a | b;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xF0, 8));
  s.add(b.raw() == ctx_.bv_val(0x0F, 8));
  s.add(c.raw() != ctx_.bv_val(0xFF, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, BitwiseXor) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Ubv<8> c = a ^ b;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xFF, 8));
  s.add(b.raw() == ctx_.bv_val(0x0F, 8));
  s.add(c.raw() != ctx_.bv_val(0xF0, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, BitwiseNot) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b = ~a;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xF0, 8));
  s.add(b.raw() != ctx_.bv_val(0x0F, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Equality ---

TEST_F(BitVecTest, Equality) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Bool eq = (a == b);

  z3::solver s(ctx_);
  s.add(eq.raw());
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, Inequality) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Bool neq = (a != b);

  z3::solver s(ctx_);
  s.add(neq.raw());
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() == ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Ordered comparison (unsigned) ---

TEST_F(BitVecTest, UnsignedLessThan) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Bool lt = (a < b);

  // 200 < 100 should be unsat (unsigned).
  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(a.raw() == ctx_.bv_val(200, 8));
  s.add(b.raw() == ctx_.bv_val(100, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, UnsignedGreaterThan) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Bool gt = (a > b);

  z3::solver s(ctx_);
  s.add(gt.raw());
  s.add(a.raw() == ctx_.bv_val(100, 8));
  s.add(b.raw() == ctx_.bv_val(200, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Ordered comparison (signed) ---

TEST_F(BitVecTest, SignedLessThan) {
  Sbv<8> a(ctx_, "a");
  Sbv<8> b(ctx_, "b");
  Bool lt = (a < b);

  // In signed 8-bit: 200 is -56, 100 is 100. So -56 < 100 is true.
  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(a.raw() == ctx_.bv_val(200, 8));  // -56 signed
  s.add(b.raw() == ctx_.bv_val(100, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

// --- Arithmetic with bit growth ---

TEST_F(BitVecTest, AdditionWidens) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  auto sum = a + b;

  // Result should be 9 bits wide.
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(!decltype(sum)::kIsSigned);
  EXPECT_EQ(sum.raw().get_sort().bv_size(), 9);
}

TEST_F(BitVecTest, AdditionNoOverflow) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  auto sum = a + b;

  // 255 + 255 = 510, which fits in 9 bits.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(255, 8));
  s.add(b.raw() == ctx_.bv_val(255, 8));
  s.add(sum.raw() != ctx_.bv_val(510, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, AdditionDifferentWidths) {
  Ubv<8> a(ctx_, "a");
  Ubv<4> b(ctx_, "b");
  auto sum = a + b;

  // Result width = max(8, 4) + 1 = 9.
  static_assert(decltype(sum)::kWidth == 9);
}

TEST_F(BitVecTest, AdditionChained) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  auto sum1 = a + b;      // Ubv<9>
  auto sum2 = sum1 + a;   // Ubv<10>

  static_assert(decltype(sum1)::kWidth == 9);
  static_assert(decltype(sum2)::kWidth == 10);
}

TEST_F(BitVecTest, SubtractionIsSigned) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  auto diff = a - b;

  static_assert(decltype(diff)::kWidth == 9);
  static_assert(decltype(diff)::kIsSigned);
}

TEST_F(BitVecTest, SubtractionCorrectValue) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  auto diff = a - b;

  // 100 - 200 = -100 in 9-bit signed.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(100, 8));
  s.add(b.raw() == ctx_.bv_val(200, 8));
  // -100 in 9-bit two's complement = 512 - 100 = 412.
  s.add(diff.raw() != ctx_.bv_val(412, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed signedness arithmetic ---

TEST_F(BitVecTest, AdditionMixedSignedness) {
  Ubv<8> a(ctx_, "a");
  Sbv<8> b(ctx_, "b");
  auto sum = a + b;

  // Result is signed when either operand is signed.
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(decltype(sum)::kIsSigned);
}

// --- Hardware shifts ---

TEST_F(BitVecTest, LeftShift) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> n(ctx_, "n");
  Ubv<8> result = a << n;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(1, 8));
  s.add(n.raw() == ctx_.bv_val(4, 8));
  s.add(result.raw() != ctx_.bv_val(16, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, UnsignedRightShift) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> n(ctx_, "n");
  Ubv<8> result = a >> n;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));
  s.add(n.raw() == ctx_.bv_val(4, 8));
  // Logical shift: 0x80 >> 4 = 0x08.
  s.add(result.raw() != ctx_.bv_val(0x08, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, SignedRightShift) {
  Sbv<8> a(ctx_, "a");
  Sbv<8> n(ctx_, "n");
  Sbv<8> result = a >> n;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));  // -128 signed
  s.add(n.raw() == ctx_.bv_val(4, 8));
  // Arithmetic shift: 0x80 >> 4 = 0xF8 (sign-extended).
  s.add(result.raw() != ctx_.bv_val(0xF8, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- ite ---

TEST_F(BitVecTest, Ite) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> b(ctx_, "b");
  Bool sel(ctx_, "sel");
  auto result = ite(sel, a, b);

  static_assert(decltype(result)::kWidth == 8);

  z3::solver s(ctx_);
  s.add(sel.raw());
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(result.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

}  // namespace
}  // namespace z3w
