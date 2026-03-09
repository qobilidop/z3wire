#include "z3wire/bitvec.h"

#include <gtest/gtest.h>
#include <z3++.h>

#include "z3wire/int.h"

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
  auto sum1 = a + b;     // Ubv<9>
  auto sum2 = sum1 + a;  // Ubv<10>

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

// --- cast ---

TEST_F(BitVecTest, CastTruncation) {
  Ubv<16> a(ctx_, "a");
  auto b = cast<Ubv<8>>(a);

  static_assert(decltype(b)::kWidth == 8);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x1234, 16));
  s.add(b.raw() != ctx_.bv_val(0x34, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, CastZeroExtension) {
  Ubv<8> a(ctx_, "a");
  auto b = cast<Ubv<16>>(a);

  static_assert(decltype(b)::kWidth == 16);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xFF, 8));
  s.add(b.raw() != ctx_.bv_val(0x00FF, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, CastSignExtension) {
  Sbv<8> a(ctx_, "a");
  auto b = cast<Sbv<16>>(a);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));     // -128
  s.add(b.raw() != ctx_.bv_val(0xFF80, 16));  // -128 sign-extended
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, CastBitcast) {
  Ubv<8> a(ctx_, "a");
  auto b = cast<Sbv<8>>(a);

  static_assert(decltype(b)::kIsSigned);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- safe_cast ---

TEST_F(BitVecTest, SafeCastWidening) {
  Ubv<8> a(ctx_, "a");
  auto b = safe_cast<Ubv<16>>(a);

  static_assert(decltype(b)::kWidth == 16);
}

TEST_F(BitVecTest, SafeCastUnsignedToSigned) {
  Ubv<8> a(ctx_, "a");
  // Needs W2 > W1 (9 > 8).
  auto b = safe_cast<Sbv<9>>(a);

  static_assert(decltype(b)::kWidth == 9);
  static_assert(decltype(b)::kIsSigned);
}

// --- checked_cast ---

TEST_F(BitVecTest, CheckedCastNoOverflow) {
  Ubv<16> a(ctx_, "a");
  auto [result, overflowed] = checked_cast<Ubv<8>>(a);

  // When value fits in 8 bits, overflow should be false.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(42, 16));
  s.add(overflowed.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, CheckedCastWithOverflow) {
  Ubv<16> a(ctx_, "a");
  auto [result, overflowed] = checked_cast<Ubv<8>>(a);

  // When value doesn't fit, overflow should be true.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(256, 16));
  s.add(!overflowed.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Bool / Ubv<1> conversion ---

TEST_F(BitVecTest, ToUbv1) {
  Bool b = Bool::True(ctx_);
  Ubv<1> v = to_ubv1(b);

  z3::solver s(ctx_);
  s.add(v.raw() != ctx_.bv_val(1, 1));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, ToBool) {
  auto v = Ubv<1>::Literal<1>(ctx_);
  Bool b = to_bool(v);

  z3::solver s(ctx_);
  s.add(!b.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, ToBoolRoundtrip) {
  Bool orig(ctx_, "orig");
  Bool roundtrip = to_bool(to_ubv1(orig));

  // orig <=> roundtrip should always hold.
  z3::solver s(ctx_);
  s.add(orig.raw() != roundtrip.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- extract ---

TEST_F(BitVecTest, StaticExtract) {
  Ubv<16> a(ctx_, "a");
  auto high = extract<15, 8>(a);
  auto low = extract<7, 0>(a);

  static_assert(decltype(high)::kWidth == 8);
  static_assert(decltype(low)::kWidth == 8);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x1234, 16));
  s.add(high.raw() != ctx_.bv_val(0x12, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, SymbolicExtract) {
  Ubv<16> a(ctx_, "a");
  Ubv<4> idx(ctx_, "idx");
  auto nibble = extract<4>(a, idx);

  static_assert(decltype(nibble)::kWidth == 4);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xABCD, 16));
  s.add(idx.raw() == ctx_.bv_val(4, 4));
  // Shift right by 4: 0xABCD >> 4 = 0x0ABC, take low 4 bits = 0xC.
  s.add(nibble.raw() != ctx_.bv_val(0xC, 4));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- concat ---

TEST_F(BitVecTest, Concat) {
  Ubv<8> high(ctx_, "high");
  Ubv<8> low(ctx_, "low");
  auto full = concat(high, low);

  static_assert(decltype(full)::kWidth == 16);

  z3::solver s(ctx_);
  s.add(high.raw() == ctx_.bv_val(0xAB, 8));
  s.add(low.raw() == ctx_.bv_val(0xCD, 8));
  s.add(full.raw() != ctx_.bv_val(0xABCD, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, ConcatVariadic) {
  Ubv<4> a(ctx_, "a");
  Ubv<4> b(ctx_, "b");
  Ubv<4> c(ctx_, "c");
  auto result = concat(a, b, c);

  static_assert(decltype(result)::kWidth == 12);
}

// --- checked_shl ---

TEST_F(BitVecTest, CheckedShlNoLoss) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> n(ctx_, "n");
  auto [shifted, lost] = checked_shl(a, n);

  // Shifting 1 left by 4 should not lose bits.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(1, 8));
  s.add(n.raw() == ctx_.bv_val(4, 8));
  s.add(lost.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, CheckedShlWithLoss) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> n(ctx_, "n");
  auto [shifted, lost] = checked_shl(a, n);

  // Shifting 0x80 left by 1 loses the high bit.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));
  s.add(n.raw() == ctx_.bv_val(1, 8));
  s.add(!lost.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- checked_shr ---

TEST_F(BitVecTest, CheckedShrWithLoss) {
  Ubv<8> a(ctx_, "a");
  Ubv<8> n(ctx_, "n");
  auto [shifted, lost] = checked_shr(a, n);

  // Shifting 0x01 right by 1 loses the low bit.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(1, 8));
  s.add(n.raw() == ctx_.bv_val(1, 8));
  s.add(!lost.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- lossless_shl ---

TEST_F(BitVecTest, LosslessShlConstant) {
  Ubv<8> a(ctx_, "a");
  auto result = lossless_shl<3>(a);

  static_assert(decltype(result)::kWidth == 11);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xFF, 8));
  // 0xFF << 3 = 0x7F8.
  s.add(result.raw() != ctx_.bv_val(0x7F8, 11));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, LosslessShlSymbolic) {
  Ubv<8> a(ctx_, "a");
  Ubv<3> n(ctx_, "n");
  auto result = lossless_shl(a, n);

  // Result width = 8 + 2^3 - 1 = 15.
  static_assert(decltype(result)::kWidth == 15);
}

// --- Type traits ---

TEST_F(BitVecTest, IsSymbolicV) {
  static_assert(is_symbolic_v<Ubv<8>>);
  static_assert(is_symbolic_v<Sbv<16>>);
  static_assert(!is_symbolic_v<int>);
}

// --- Concrete to symbolic promotion ---

TEST_F(BitVecTest, ConcreteToSymbolic) {
  UInt<8> concrete(42);
  Ubv<8> symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, SignedConcreteToSymbolic) {
  SInt<8> concrete(0x80);  // -128
  Sbv<8> symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.raw() != ctx_.bv_val(0x80, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed concrete + symbolic arithmetic ---

TEST_F(BitVecTest, MixedAddSymbolicPlusConcrete) {
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(42);
  auto result = sym + conc;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(is_symbolic_v<decltype(result)>);

  z3::solver s(ctx_);
  s.add(sym.raw() == ctx_.bv_val(10, 8));
  s.add(result.raw() != ctx_.bv_val(52, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BitVecTest, MixedAddConcretePlusSymbolic) {
  UInt<8> conc(42);
  Ubv<8> sym(ctx_, "x");
  auto result = conc + sym;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(is_symbolic_v<decltype(result)>);
}

TEST_F(BitVecTest, MixedSubSymbolicMinusConcrete) {
  Ubv<8> sym(ctx_, "x");
  UInt<8> conc(50);
  auto result = sym - conc;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(decltype(result)::kIsSigned);
  static_assert(is_symbolic_v<decltype(result)>);
}

TEST_F(BitVecTest, MixedAddDifferentWidths) {
  Ubv<8> sym(ctx_, "x");
  UInt<4> conc(15);
  auto result = sym + conc;

  static_assert(decltype(result)::kWidth == 9);
}

}  // namespace
}  // namespace z3w
