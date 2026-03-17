#include "z3wire/sym_bit_vec.h"

#include <gtest/gtest.h>
#include <z3++.h>

#include "z3wire/bit_vec.h"

namespace z3w {
namespace {

class SymBitVecTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

// --- Construction ---

TEST_F(SymBitVecTest, SymbolicVariable) {
  SymUInt<8> a(ctx_, "a");
  EXPECT_EQ(a.raw().get_sort().bv_size(), 8);
}

TEST_F(SymBitVecTest, Literal) {
  auto val = SymUInt<8>::Literal<255>(ctx_);
  z3::solver s(ctx_);
  s.add(val.raw() == ctx_.bv_val(255, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(SymBitVecTest, LiteralZero) {
  auto val = SymUInt<8>::Literal<0>(ctx_);
  z3::solver s(ctx_);
  s.add(val.raw() == ctx_.bv_val(0, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

// --- Bitwise ---

TEST_F(SymBitVecTest, BitwiseAnd) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymUInt<8> c = a & b;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xF0, 8));
  s.add(b.raw() == ctx_.bv_val(0x3C, 8));
  s.add(c.raw() != ctx_.bv_val(0x30, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, BitwiseOr) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymUInt<8> c = a | b;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xF0, 8));
  s.add(b.raw() == ctx_.bv_val(0x0F, 8));
  s.add(c.raw() != ctx_.bv_val(0xFF, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, BitwiseXor) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymUInt<8> c = a ^ b;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xFF, 8));
  s.add(b.raw() == ctx_.bv_val(0x0F, 8));
  s.add(c.raw() != ctx_.bv_val(0xF0, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, BitwiseNot) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b = ~a;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xF0, 8));
  s.add(b.raw() != ctx_.bv_val(0x0F, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Exact equality ---

TEST_F(SymBitVecTest, ExactEq) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool eq = exact_eq(a, b);

  z3::solver s(ctx_);
  s.add(eq.raw());
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Equality ---

TEST_F(SymBitVecTest, Equality) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool eq = (a == b);

  z3::solver s(ctx_);
  s.add(eq.raw());
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, Inequality) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool neq = (a != b);

  z3::solver s(ctx_);
  s.add(neq.raw());
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() == ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Ordered comparison (unsigned) ---

TEST_F(SymBitVecTest, UnsignedLessThan) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool lt = (a < b);

  // 200 < 100 should be unsat (unsigned).
  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(a.raw() == ctx_.bv_val(200, 8));
  s.add(b.raw() == ctx_.bv_val(100, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, UnsignedGreaterThan) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool gt = (a > b);

  z3::solver s(ctx_);
  s.add(gt.raw());
  s.add(a.raw() == ctx_.bv_val(100, 8));
  s.add(b.raw() == ctx_.bv_val(200, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Ordered comparison (signed) ---

TEST_F(SymBitVecTest, SignedLessThan) {
  SymSInt<8> a(ctx_, "a");
  SymSInt<8> b(ctx_, "b");
  SymBool lt = (a < b);

  // In signed 8-bit: 200 is -56, 100 is 100. So -56 < 100 is true.
  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(a.raw() == ctx_.bv_val(200, 8));  // -56 signed
  s.add(b.raw() == ctx_.bv_val(100, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

// --- Arithmetic with bit growth ---

TEST_F(SymBitVecTest, AdditionWidens) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto sum = a + b;

  // Result should be 9 bits wide.
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(!decltype(sum)::kIsSigned);
  EXPECT_EQ(sum.raw().get_sort().bv_size(), 9);
}

TEST_F(SymBitVecTest, AdditionNoOverflow) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto sum = a + b;

  // 255 + 255 = 510, which fits in 9 bits.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(255, 8));
  s.add(b.raw() == ctx_.bv_val(255, 8));
  s.add(sum.raw() != ctx_.bv_val(510, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, AdditionDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<4> b(ctx_, "b");
  auto sum = a + b;

  // Result width = max(8, 4) + 1 = 9.
  static_assert(decltype(sum)::kWidth == 9);
}

TEST_F(SymBitVecTest, AdditionChained) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto sum1 = a + b;     // SymUInt<9>
  auto sum2 = sum1 + a;  // SymUInt<10>

  static_assert(decltype(sum1)::kWidth == 9);
  static_assert(decltype(sum2)::kWidth == 10);
}

TEST_F(SymBitVecTest, SubtractionIsSigned) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto diff = a - b;

  static_assert(decltype(diff)::kWidth == 9);
  static_assert(decltype(diff)::kIsSigned);
}

TEST_F(SymBitVecTest, SubtractionCorrectValue) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
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

TEST_F(SymBitVecTest, AdditionMixedSignedness) {
  SymUInt<8> a(ctx_, "a");
  SymSInt<8> b(ctx_, "b");
  auto sum = a + b;

  // Result is signed when either operand is signed.
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(decltype(sum)::kIsSigned);
}

// --- Hardware shifts ---

TEST_F(SymBitVecTest, LeftShift) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  SymUInt<8> result = a << n;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(1, 8));
  s.add(n.raw() == ctx_.bv_val(4, 8));
  s.add(result.raw() != ctx_.bv_val(16, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, UnsignedRightShift) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  SymUInt<8> result = a >> n;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));
  s.add(n.raw() == ctx_.bv_val(4, 8));
  // Logical shift: 0x80 >> 4 = 0x08.
  s.add(result.raw() != ctx_.bv_val(0x08, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SignedRightShift) {
  SymSInt<8> a(ctx_, "a");
  SymSInt<8> n(ctx_, "n");
  SymSInt<8> result = a >> n;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));  // -128 signed
  s.add(n.raw() == ctx_.bv_val(4, 8));
  // Arithmetic shift: 0x80 >> 4 = 0xF8 (sign-extended).
  s.add(result.raw() != ctx_.bv_val(0xF8, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- ite ---

TEST_F(SymBitVecTest, Ite) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool sel(ctx_, "sel");
  auto result = ite(sel, a, b);

  static_assert(decltype(result)::kWidth == 8);

  z3::solver s(ctx_);
  s.add(sel.raw());
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(result.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- unsafe_cast ---

TEST_F(SymBitVecTest, CastTruncation) {
  SymUInt<16> a(ctx_, "a");
  auto b = unsafe_cast<SymUInt<8>>(a);

  static_assert(decltype(b)::kWidth == 8);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x1234, 16));
  s.add(b.raw() != ctx_.bv_val(0x34, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CastZeroExtension) {
  SymUInt<8> a(ctx_, "a");
  auto b = unsafe_cast<SymUInt<16>>(a);

  static_assert(decltype(b)::kWidth == 16);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xFF, 8));
  s.add(b.raw() != ctx_.bv_val(0x00FF, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CastSignExtension) {
  SymSInt<8> a(ctx_, "a");
  auto b = unsafe_cast<SymSInt<16>>(a);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));     // -128
  s.add(b.raw() != ctx_.bv_val(0xFF80, 16));  // -128 sign-extended
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CastBitcast) {
  SymUInt<8> a(ctx_, "a");
  auto b = unsafe_cast<SymSInt<8>>(a);

  static_assert(decltype(b)::kIsSigned);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- safe_cast ---

TEST_F(SymBitVecTest, SafeCastWidening) {
  SymUInt<8> a(ctx_, "a");
  auto b = safe_cast<SymUInt<16>>(a);

  static_assert(decltype(b)::kWidth == 16);
}

TEST_F(SymBitVecTest, SafeCastUnsignedToSigned) {
  SymUInt<8> a(ctx_, "a");
  // Needs W2 > W1 (9 > 8).
  auto b = safe_cast<SymSInt<9>>(a);

  static_assert(decltype(b)::kWidth == 9);
  static_assert(decltype(b)::kIsSigned);
}

// --- checked_cast ---

TEST_F(SymBitVecTest, CheckedCastValuePreserved) {
  SymUInt<16> a(ctx_, "a");
  auto [result, value_preserved] = checked_cast<SymUInt<8>>(a);

  // When value fits in 8 bits, value_preserved should be true.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(42, 16));
  s.add(!value_preserved.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CheckedCastValueNotPreserved) {
  SymUInt<16> a(ctx_, "a");
  auto [result, value_preserved] = checked_cast<SymUInt<8>>(a);

  // When value doesn't fit, value_preserved should be false.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(256, 16));
  s.add(value_preserved.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- SymBool / SymUInt<1> conversion ---

TEST_F(SymBitVecTest, ToUbv1) {
  SymBool b = SymBool::True(ctx_);
  SymUInt<1> v = to_ubv1(b);

  z3::solver s(ctx_);
  s.add(v.raw() != ctx_.bv_val(1, 1));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ToBool) {
  auto v = SymUInt<1>::Literal<1>(ctx_);
  SymBool b = to_bool(v);

  z3::solver s(ctx_);
  s.add(!b.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ToBoolRoundtrip) {
  SymBool orig(ctx_, "orig");
  SymBool roundtrip = to_bool(to_ubv1(orig));

  // orig <=> roundtrip should always hold.
  z3::solver s(ctx_);
  s.add(orig.raw() != roundtrip.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- extract ---

TEST_F(SymBitVecTest, StaticExtract) {
  SymUInt<16> a(ctx_, "a");
  auto high = extract<15, 8>(a);
  auto low = extract<7, 0>(a);

  static_assert(decltype(high)::kWidth == 8);
  static_assert(decltype(low)::kWidth == 8);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x1234, 16));
  s.add(high.raw() != ctx_.bv_val(0x12, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SymbolicExtract) {
  SymUInt<16> a(ctx_, "a");
  SymUInt<4> idx(ctx_, "idx");
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

TEST_F(SymBitVecTest, Concat) {
  SymUInt<8> high(ctx_, "high");
  SymUInt<8> low(ctx_, "low");
  auto full = concat(high, low);

  static_assert(decltype(full)::kWidth == 16);

  z3::solver s(ctx_);
  s.add(high.raw() == ctx_.bv_val(0xAB, 8));
  s.add(low.raw() == ctx_.bv_val(0xCD, 8));
  s.add(full.raw() != ctx_.bv_val(0xABCD, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ConcatVariadic) {
  SymUInt<4> a(ctx_, "a");
  SymUInt<4> b(ctx_, "b");
  SymUInt<4> c(ctx_, "c");
  auto result = concat(a, b, c);

  static_assert(decltype(result)::kWidth == 12);
}

// --- checked_shl ---

TEST_F(SymBitVecTest, CheckedShlNoLoss) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  auto [shifted, lost] = checked_shl(a, n);

  // Shifting 1 left by 4 should not lose bits.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(1, 8));
  s.add(n.raw() == ctx_.bv_val(4, 8));
  s.add(lost.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CheckedShlWithLoss) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  auto [shifted, lost] = checked_shl(a, n);

  // Shifting 0x80 left by 1 loses the high bit.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));
  s.add(n.raw() == ctx_.bv_val(1, 8));
  s.add(!lost.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- checked_shr ---

TEST_F(SymBitVecTest, CheckedShrWithLoss) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  auto [shifted, lost] = checked_shr(a, n);

  // Shifting 0x01 right by 1 loses the low bit.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(1, 8));
  s.add(n.raw() == ctx_.bv_val(1, 8));
  s.add(!lost.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- lossless_shl ---

TEST_F(SymBitVecTest, LosslessShlConstant) {
  SymUInt<8> a(ctx_, "a");
  auto result = lossless_shl<3>(a);

  static_assert(decltype(result)::kWidth == 11);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xFF, 8));
  // 0xFF << 3 = 0x7F8.
  s.add(result.raw() != ctx_.bv_val(0x7F8, 11));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, LosslessShlSymbolic) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<3> n(ctx_, "n");
  auto result = lossless_shl(a, n);

  // Result width = 8 + 2^3 - 1 = 15.
  static_assert(decltype(result)::kWidth == 15);
}

// --- Type traits ---

TEST_F(SymBitVecTest, IsSymbolicV) {
  static_assert(is_symbolic_v<SymUInt<8>>);
  static_assert(is_symbolic_v<SymSInt<16>>);
  static_assert(!is_symbolic_v<int>);
}

// --- Concrete to symbolic promotion ---

TEST_F(SymBitVecTest, ConcreteToSymbolic) {
  auto concrete = UInt<8>::Literal<42>();
  SymUInt<8> symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SignedConcreteToSymbolic) {
  auto concrete = SInt<8>::Literal<-128>();
  SymSInt<8> symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.raw() != ctx_.bv_val(0x80, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed concrete + symbolic arithmetic ---

TEST_F(SymBitVecTest, MixedAddSymbolicPlusConcrete) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<42>();
  auto result = sym + conc;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(is_symbolic_v<decltype(result)>);

  z3::solver s(ctx_);
  s.add(sym.raw() == ctx_.bv_val(10, 8));
  s.add(result.raw() != ctx_.bv_val(52, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedAddConcretePlusSymbolic) {
  auto conc = UInt<8>::Literal<42>();
  SymUInt<8> sym(ctx_, "x");
  auto result = conc + sym;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(is_symbolic_v<decltype(result)>);
}

TEST_F(SymBitVecTest, MixedSubSymbolicMinusConcrete) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<50>();
  auto result = sym - conc;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(decltype(result)::kIsSigned);
  static_assert(is_symbolic_v<decltype(result)>);
}

TEST_F(SymBitVecTest, MixedAddDifferentWidths) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<4>::Literal<15>();
  auto result = sym + conc;

  static_assert(decltype(result)::kWidth == 9);
}

// --- Mixed bitwise ---

TEST_F(SymBitVecTest, MixedBitwiseAnd) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<0x0F>();
  SymUInt<8> result = sym & conc;

  z3::solver s(ctx_);
  s.add(sym.raw() == ctx_.bv_val(0xAB, 8));
  s.add(result.raw() != ctx_.bv_val(0x0B, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed comparison ---

TEST_F(SymBitVecTest, MixedEquality) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<42>();
  SymBool eq = (sym == conc);

  z3::solver s(ctx_);
  s.add(eq.raw());
  s.add(sym.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedLessThan) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<100>();
  SymBool lt = (sym < conc);

  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(sym.raw() == ctx_.bv_val(200, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed ite ---

TEST_F(SymBitVecTest, MixedIteSymbolicCondConcreteValues) {
  SymBool cond(ctx_, "c");
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<8>::Literal<99>();
  auto result = ite(cond, a, b);

  static_assert(is_symbolic_v<decltype(result)>);

  z3::solver s(ctx_);
  s.add(cond.raw());
  s.add(result.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedIteSymbolicCondMixedValues) {
  SymBool cond(ctx_, "c");
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<99>();
  auto result = ite(cond, sym, conc);

  static_assert(is_symbolic_v<decltype(result)>);
}

// --- Mixed shifts ---

TEST_F(SymBitVecTest, MixedLeftShift) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<4>();
  auto result = sym << conc;

  z3::solver s(ctx_);
  s.add(sym.raw() == ctx_.bv_val(1, 8));
  s.add(result.raw() != ctx_.bv_val(16, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedRightShift) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<4>();
  auto result = sym >> conc;

  z3::solver s(ctx_);
  s.add(sym.raw() == ctx_.bv_val(0x80, 8));
  s.add(result.raw() != ctx_.bv_val(0x08, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- is_symbolic_v<SymBool> ---

TEST_F(SymBitVecTest, IsSymbolicBool) { static_assert(is_symbolic_v<SymBool>); }

// --- Cross-type symbolic comparison (different widths) ---

TEST_F(SymBitVecTest, CrossTypeEqualityDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<16> b(ctx_, "b");
  SymBool eq = (a == b);

  // If a=42 and b=42, they should be equal.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() == ctx_.bv_val(42, 16));
  s.add(!eq.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CrossTypeInequalityDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<16> b(ctx_, "b");
  SymBool neq = (a != b);

  // If a=42 and b=42, they should not be unequal.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(42, 8));
  s.add(b.raw() == ctx_.bv_val(42, 16));
  s.add(neq.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CrossTypeLessThanDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<16> b(ctx_, "b");
  SymBool lt = (a < b);

  // 100 < 200 should be sat (unsigned).
  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(a.raw() == ctx_.bv_val(100, 8));
  s.add(b.raw() == ctx_.bv_val(200, 16));
  EXPECT_EQ(s.check(), z3::sat);
}

// --- Cross-type symbolic comparison (different signedness) ---

TEST_F(SymBitVecTest, CrossTypeLessThanDifferentSignedness) {
  // SymSInt<8> -1 (0xFF) < SymUInt<8> 200 should be true.
  // Common type: signed, width = max(8,8)+1 = 9.
  SymSInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool lt = (a < b);

  z3::solver s(ctx_);
  s.add(lt.raw());
  s.add(a.raw() == ctx_.bv_val(0xFF, 8));  // -1 signed
  s.add(b.raw() == ctx_.bv_val(200, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(SymBitVecTest, CrossTypeGreaterEqualDifferentSignedness) {
  // SymUInt<8> 200 >= SymSInt<8> -1 should be true.
  SymUInt<8> a(ctx_, "a");
  SymSInt<8> b(ctx_, "b");
  SymBool ge = (a >= b);

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(200, 8));
  s.add(b.raw() == ctx_.bv_val(0xFF, 8));  // -1 signed
  s.add(!ge.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed concrete+symbolic cross-type comparison ---

TEST_F(SymBitVecTest, MixedCrossTypeEquality) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<16>::Literal<42>();
  SymBool eq = (sym == conc);

  z3::solver s(ctx_);
  s.add(eq.raw());
  s.add(sym.raw() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedCrossTypeLessThan) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<16>::Literal<300>();
  SymBool lt = (sym < conc);

  // Any 8-bit unsigned value (0-255) is always < 300.
  z3::solver s(ctx_);
  s.add(!lt.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Unary negate ---

TEST_F(SymBitVecTest, NegateUnsigned) {
  SymUInt<8> a(ctx_, "a");
  auto neg = -a;

  // Result: width = 9, always signed.
  static_assert(decltype(neg)::kWidth == 9);
  static_assert(decltype(neg)::kIsSigned);

  // -100 in 9-bit two's complement = 412.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(100, 8));
  s.add(neg.raw() != ctx_.bv_val(412, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, NegateSigned) {
  SymSInt<8> a(ctx_, "a");
  auto neg = -a;

  static_assert(decltype(neg)::kWidth == 9);
  static_assert(decltype(neg)::kIsSigned);

  // Negate -128 -> 128, which fits in SymSInt<9>.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0x80, 8));  // -128 signed
  s.add(neg.raw() != ctx_.bv_val(128, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, NegateZero) {
  SymUInt<8> a(ctx_, "a");
  auto neg = -a;

  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0, 8));
  s.add(neg.raw() != ctx_.bv_val(0, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed unary negate ---

TEST_F(SymBitVecTest, NegateConsistentWithBinarySub) {
  SymUInt<8> a(ctx_, "a");
  auto neg = -a;
  auto zero_minus_a = SymUInt<8>::Literal<0>(ctx_) - a;

  // -a should equal 0 - a for all values.
  z3::solver s(ctx_);
  s.add(neg.raw() != zero_minus_a.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Single-bit extraction ---

TEST_F(SymBitVecTest, BitExtract) {
  SymUInt<8> a(ctx_, "a");
  auto b5 = bit<5>(a);

  static_assert(decltype(b5)::kWidth == 1);

  // 0xA5 = 10100101, bit 5 = 1.
  z3::solver s(ctx_);
  s.add(a.raw() == ctx_.bv_val(0xA5, 8));
  s.add(b5.raw() != ctx_.bv_val(1, 1));
  EXPECT_EQ(s.check(), z3::unsat);
}

}  // namespace
}  // namespace z3w
