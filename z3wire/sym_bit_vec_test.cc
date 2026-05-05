#include "z3wire/sym_bit_vec.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>
#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/sym_bool.h"
#include "z3wire/type_traits.h"

namespace z3w {
namespace {

class SymBitVecTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

// --- Construction ---

TEST_F(SymBitVecTest, SymbolicVariable) {
  SymUInt<8> a(ctx_, "a");
  EXPECT_EQ(a.expr().get_sort().bv_size(), 8);
}

TEST_F(SymBitVecTest, Literal) {
  auto val = SymUInt<8>::Literal<255>(ctx_);
  z3::solver s(ctx_);
  s.add(val.expr() == ctx_.bv_val(255, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(SymBitVecTest, LiteralZero) {
  auto val = SymUInt<8>::Literal<0>(ctx_);
  z3::solver s(ctx_);
  s.add(val.expr() == ctx_.bv_val(0, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

// --- Bitwise ---

TEST_F(SymBitVecTest, BitwiseAnd) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymUInt<8> c = a & b;

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xF0, 8));
  s.add(b.expr() == ctx_.bv_val(0x3C, 8));
  s.add(c.expr() != ctx_.bv_val(0x30, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, BitwiseOr) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymUInt<8> c = a | b;

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xF0, 8));
  s.add(b.expr() == ctx_.bv_val(0x0F, 8));
  s.add(c.expr() != ctx_.bv_val(0xFF, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, BitwiseXor) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymUInt<8> c = a ^ b;

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xFF, 8));
  s.add(b.expr() == ctx_.bv_val(0x0F, 8));
  s.add(c.expr() != ctx_.bv_val(0xF0, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, BitwiseNot) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b = ~a;

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xF0, 8));
  s.add(b.expr() != ctx_.bv_val(0x0F, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Equality ---

TEST_F(SymBitVecTest, Equality) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool eq = (a == b);

  z3::solver s(ctx_);
  s.add(eq.expr());
  s.add(a.expr() == ctx_.bv_val(42, 8));
  s.add(b.expr() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, Inequality) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool neq = (a != b);

  z3::solver s(ctx_);
  s.add(neq.expr());
  s.add(a.expr() == ctx_.bv_val(42, 8));
  s.add(b.expr() == ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Ordered comparison (unsigned) ---

TEST_F(SymBitVecTest, UnsignedLessThan) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool lt = (a < b);

  // 200 < 100 should be unsat (unsigned).
  z3::solver s(ctx_);
  s.add(lt.expr());
  s.add(a.expr() == ctx_.bv_val(200, 8));
  s.add(b.expr() == ctx_.bv_val(100, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, UnsignedGreaterThan) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  SymBool gt = (a > b);

  z3::solver s(ctx_);
  s.add(gt.expr());
  s.add(a.expr() == ctx_.bv_val(100, 8));
  s.add(b.expr() == ctx_.bv_val(200, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Ordered comparison (signed) ---

TEST_F(SymBitVecTest, SignedLessThan) {
  SymSInt<8> a(ctx_, "a");
  SymSInt<8> b(ctx_, "b");
  SymBool lt = (a < b);

  // In signed 8-bit: 200 is -56, 100 is 100. So -56 < 100 is true.
  z3::solver s(ctx_);
  s.add(lt.expr());
  s.add(a.expr() == ctx_.bv_val(200, 8));  // -56 signed
  s.add(b.expr() == ctx_.bv_val(100, 8));
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
  EXPECT_EQ(sum.expr().get_sort().bv_size(), 9);
}

TEST_F(SymBitVecTest, AdditionNoOverflow) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto sum = a + b;

  // 255 + 255 = 510, which fits in 9 bits.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(255, 8));
  s.add(b.expr() == ctx_.bv_val(255, 8));
  s.add(sum.expr() != ctx_.bv_val(510, 9));
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
  s.add(a.expr() == ctx_.bv_val(100, 8));
  s.add(b.expr() == ctx_.bv_val(200, 8));
  // -100 in 9-bit two's complement = 512 - 100 = 412.
  s.add(diff.expr() != ctx_.bv_val(412, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed signedness arithmetic ---

TEST_F(SymBitVecTest, AdditionMixedSignedness) {
  SymUInt<8> a(ctx_, "a");
  SymSInt<8> b(ctx_, "b");
  auto sum = a + b;

  // CIRCT hwarith rule: ui<8> + si<8> -> si<10> (A+2 since A >= B).
  static_assert(decltype(sum)::kWidth == 10);
  static_assert(decltype(sum)::kIsSigned);
}

// --- Multiplication with bit growth ---

TEST_F(SymBitVecTest, MultiplicationWidens) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto product = a * b;

  // Result width = 8 + 8 = 16.
  static_assert(decltype(product)::kWidth == 16);
  static_assert(!decltype(product)::kIsSigned);
  EXPECT_EQ(product.expr().get_sort().bv_size(), 16);
}

TEST_F(SymBitVecTest, MultiplicationDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<4> b(ctx_, "b");
  auto product = a * b;

  // Result width = 8 + 4 = 12.
  static_assert(decltype(product)::kWidth == 12);
  static_assert(!decltype(product)::kIsSigned);
}

TEST_F(SymBitVecTest, MultiplicationSignedResult) {
  SymSInt<8> a(ctx_, "a");
  SymSInt<8> b(ctx_, "b");
  auto product = a * b;

  static_assert(decltype(product)::kWidth == 16);
  static_assert(decltype(product)::kIsSigned);
}

TEST_F(SymBitVecTest, MultiplicationMixedSignedness) {
  SymUInt<4> a(ctx_, "a");
  SymSInt<4> b(ctx_, "b");
  auto product = a * b;

  // ui<4> * si<4> -> si<8>, signed if either is signed.
  static_assert(decltype(product)::kWidth == 8);
  static_assert(decltype(product)::kIsSigned);
}

TEST_F(SymBitVecTest, MultiplicationCorrectValue) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto product = a * b;

  // 15 * 15 = 225.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(15, 8));
  s.add(b.expr() == ctx_.bv_val(15, 8));
  s.add(product.expr() != ctx_.bv_val(225, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MultiplicationNoOverflow) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> b(ctx_, "b");
  auto product = a * b;

  // 255 * 255 = 65025, which fits in 16 bits.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(255, 8));
  s.add(b.expr() == ctx_.bv_val(255, 8));
  s.add(product.expr() != ctx_.bv_val(65025, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MultiplicationMixedSignednessCorrectValue) {
  SymUInt<4> a(ctx_, "a");
  SymSInt<4> b(ctx_, "b");
  auto product = a * b;

  // ui<4>(15) * si<4>(-2) = -30 in si<8>.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(15, 4));
  s.add(b.expr() == ctx_.bv_val(14, 4));  // -2 in 4-bit two's complement
  // -30 in 8-bit two's complement = 256 - 30 = 226.
  s.add(product.expr() != ctx_.bv_val(226, 8));
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
  s.add(sel.expr());
  s.add(a.expr() == ctx_.bv_val(42, 8));
  s.add(result.expr() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, IteSymBool) {
  SymBool sel(ctx_, "sel");
  SymBool a(ctx_, "a");
  SymBool b(ctx_, "b");
  SymBool result = ite(sel, a, b);

  // sel=true -> result=a
  z3::solver s(ctx_);
  s.add(sel.expr());
  s.add(a.expr());
  s.add(!result.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- unsafe_cast ---

TEST_F(SymBitVecTest, CastTruncation) {
  SymUInt<16> a(ctx_, "a");
  auto b = unsafe_cast<SymUInt<8>>(a);

  static_assert(decltype(b)::kWidth == 8);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x1234, 16));
  s.add(b.expr() != ctx_.bv_val(0x34, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CastZeroExtension) {
  SymUInt<8> a(ctx_, "a");
  auto b = unsafe_cast<SymUInt<16>>(a);

  static_assert(decltype(b)::kWidth == 16);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xFF, 8));
  s.add(b.expr() != ctx_.bv_val(0x00FF, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CastSignExtension) {
  SymSInt<8> a(ctx_, "a");
  auto b = unsafe_cast<SymSInt<16>>(a);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));     // -128
  s.add(b.expr() != ctx_.bv_val(0xFF80, 16));  // -128 sign-extended
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CastBitcast) {
  SymUInt<8> a(ctx_, "a");
  auto b = unsafe_cast<SymSInt<8>>(a);

  static_assert(decltype(b)::kIsSigned);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(42, 8));
  s.add(b.expr() != ctx_.bv_val(42, 8));
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
  s.add(a.expr() == ctx_.bv_val(42, 16));
  s.add(!value_preserved.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CheckedCastValueNotPreserved) {
  SymUInt<16> a(ctx_, "a");
  auto [result, value_preserved] = checked_cast<SymUInt<8>>(a);

  // When value doesn't fit, value_preserved should be false.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(256, 16));
  s.add(value_preserved.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- as_signed / as_unsigned ---

TEST_F(SymBitVecTest, AsUnsigned) {
  SymSInt<8> a(ctx_, "a");
  auto result = as_unsigned(a);

  static_assert(std::is_same_v<decltype(result), SymUInt<8>>);
  static_assert(decltype(result)::kWidth == 8);
  static_assert(!decltype(result)::kIsSigned);

  // Same bits: -1 signed = 0xFF = 255 unsigned.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xFF, 8));
  s.add(result.expr() != ctx_.bv_val(0xFF, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, AsSigned) {
  SymUInt<8> a(ctx_, "a");
  auto result = as_signed(a);

  static_assert((std::is_same_v<decltype(result), SymSInt<8>>));
  static_assert(decltype(result)::kWidth == 8);
  static_assert(decltype(result)::kIsSigned);

  // Same bits: 0xFF unsigned = -1 signed.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xFF, 8));
  s.add(result.expr() != ctx_.bv_val(0xFF, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- SymBool / SymUInt<1> conversion ---

TEST_F(SymBitVecTest, ToUbv1) {
  SymBool b = SymBool::True(ctx_);
  SymUInt<1> v = as_uint1(b);

  z3::solver s(ctx_);
  s.add(v.expr() != ctx_.bv_val(1, 1));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ToBool) {
  auto v = SymUInt<1>::Literal<1>(ctx_);
  SymBool b = as_bool(v);

  z3::solver s(ctx_);
  s.add(!b.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ToBoolRoundtrip) {
  SymBool orig(ctx_, "orig");
  SymBool roundtrip = as_bool(as_uint1(orig));

  // orig <=> roundtrip should always hold.
  z3::solver s(ctx_);
  s.add(orig.expr() != roundtrip.expr());
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
  s.add(a.expr() == ctx_.bv_val(0x1234, 16));
  s.add(high.expr() != ctx_.bv_val(0x12, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SymbolicExtract) {
  SymUInt<16> a(ctx_, "a");
  SymUInt<4> idx(ctx_, "idx");
  auto nibble = extract<4>(a, idx);

  static_assert(decltype(nibble)::kWidth == 4);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xABCD, 16));
  s.add(idx.expr() == ctx_.bv_val(4, 4));
  // Shift right by 4: 0xABCD >> 4 = 0x0ABC, take low 4 bits = 0xC.
  s.add(nibble.expr() != ctx_.bv_val(0xC, 4));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SymbolicExtractWideIndex) {
  // Index wider than source: IdxW (16) > W (8).
  SymUInt<8> a(ctx_, "a");
  SymUInt<16> idx(ctx_, "idx");
  auto nibble = extract<4>(a, idx);

  static_assert(decltype(nibble)::kWidth == 4);

  // In-range offset: 0xAB >> 4 = 0x0A, low 4 bits = 0xA.
  {
    z3::solver s(ctx_);
    s.add(a.expr() == ctx_.bv_val(0xAB, 8));
    s.add(idx.expr() == ctx_.bv_val(4, 16));
    s.add(nibble.expr() != ctx_.bv_val(0xA, 4));
    EXPECT_EQ(s.check(), z3::unsat);
  }

  // Out-of-range offset: shifting by 100 should yield zero (zero-extension).
  {
    z3::solver s(ctx_);
    s.add(a.expr() == ctx_.bv_val(0xAB, 8));
    s.add(idx.expr() == ctx_.bv_val(100, 16));
    s.add(nibble.expr() != ctx_.bv_val(0, 4));
    EXPECT_EQ(s.check(), z3::unsat);
  }

  // Solver must not find an out-of-range offset that produces non-zero.
  // For any offset >= W (8), the result should be zero.
  {
    z3::solver s(ctx_);
    s.add(a.expr() == ctx_.bv_val(0xFF, 8));
    s.add(z3::uge(idx.expr(), ctx_.bv_val(8, 16)));
    s.add(nibble.expr() != ctx_.bv_val(0, 4));
    EXPECT_EQ(s.check(), z3::unsat);
  }
}

// --- replace ---

TEST_F(SymBitVecTest, StaticReplace) {
  SymUInt<16> src(ctx_, "src");
  SymUInt<4> field(ctx_, "field");
  auto result = replace<4>(src, field);

  static_assert(decltype(result)::kWidth == 16);
  static_assert(!decltype(result)::kIsSigned);

  // src=0xABCD, field=0xF, replace at bit 4 -> 0xABFD
  // Bits [7:4] of 0xABCD are 0xC, replaced with 0xF -> 0xABFD
  z3::solver s(ctx_);
  s.add(src.expr() == ctx_.bv_val(0xABCD, 16));
  s.add(field.expr() == ctx_.bv_val(0xF, 4));
  s.add(result.expr() != ctx_.bv_val(0xABFD, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, StaticReplacePreservesSignedness) {
  SymSInt<16> src(ctx_, "src");
  SymUInt<4> field(ctx_, "field");
  auto result = replace<0>(src, field);

  static_assert(decltype(result)::kWidth == 16);
  static_assert(decltype(result)::kIsSigned);
}

TEST_F(SymBitVecTest, StaticReplaceLowBits) {
  SymUInt<8> src(ctx_, "src");
  SymUInt<4> field(ctx_, "field");
  auto result = replace<0>(src, field);

  // src=0xA0, field=0x5, replace at bit 0 -> 0xA5
  z3::solver s(ctx_);
  s.add(src.expr() == ctx_.bv_val(0xA0, 8));
  s.add(field.expr() == ctx_.bv_val(0x5, 4));
  s.add(result.expr() != ctx_.bv_val(0xA5, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, StaticReplaceHighBits) {
  SymUInt<8> src(ctx_, "src");
  SymUInt<4> field(ctx_, "field");
  auto result = replace<4>(src, field);

  // src=0x0D, field=0xA, replace at bit 4 -> 0xAD
  z3::solver s(ctx_);
  s.add(src.expr() == ctx_.bv_val(0x0D, 8));
  s.add(field.expr() == ctx_.bv_val(0xA, 4));
  s.add(result.expr() != ctx_.bv_val(0xAD, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SymbolicReplace) {
  SymUInt<16> src(ctx_, "src");
  SymUInt<4> field(ctx_, "field");
  SymUInt<4> lo(ctx_, "lo");
  auto result = replace(src, field, lo);

  static_assert(decltype(result)::kWidth == 16);

  // src=0xABCD, field=0xF, lo=4 -> 0xABFD (same as static test)
  z3::solver s(ctx_);
  s.add(src.expr() == ctx_.bv_val(0xABCD, 16));
  s.add(field.expr() == ctx_.bv_val(0xF, 4));
  s.add(lo.expr() == ctx_.bv_val(4, 4));
  s.add(result.expr() != ctx_.bv_val(0xABFD, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SymbolicReplacePreservesSignedness) {
  SymSInt<16> src(ctx_, "src");
  SymUInt<4> field(ctx_, "field");
  SymUInt<4> lo(ctx_, "lo");
  auto result = replace(src, field, lo);

  static_assert(decltype(result)::kWidth == 16);
  static_assert(decltype(result)::kIsSigned);
}

TEST_F(SymBitVecTest, SymbolicReplaceWideOffset) {
  // Offset wider than source: WL (16) > WS (8).
  SymUInt<8> src(ctx_, "src");
  SymUInt<4> field(ctx_, "field");
  SymUInt<16> lo(ctx_, "lo");
  auto result = replace(src, field, lo);

  static_assert(decltype(result)::kWidth == 8);

  // In-range offset: src=0xAB, field=0xF, lo=4 -> 0xFB.
  {
    z3::solver s(ctx_);
    s.add(src.expr() == ctx_.bv_val(0xAB, 8));
    s.add(field.expr() == ctx_.bv_val(0xF, 4));
    s.add(lo.expr() == ctx_.bv_val(4, 16));
    s.add(result.expr() != ctx_.bv_val(0xFB, 8));
    EXPECT_EQ(s.check(), z3::unsat);
  }

  // Out-of-range offset: replacing at offset 100 should leave src unchanged.
  {
    z3::solver s(ctx_);
    s.add(src.expr() == ctx_.bv_val(0xAB, 8));
    s.add(field.expr() == ctx_.bv_val(0xF, 4));
    s.add(lo.expr() == ctx_.bv_val(100, 16));
    s.add(result.expr() != ctx_.bv_val(0xAB, 8));
    EXPECT_EQ(s.check(), z3::unsat);
  }
}

// --- concat ---

TEST_F(SymBitVecTest, Concat) {
  SymUInt<8> high(ctx_, "high");
  SymUInt<8> low(ctx_, "low");
  auto full = concat(high, low);

  static_assert(decltype(full)::kWidth == 16);

  z3::solver s(ctx_);
  s.add(high.expr() == ctx_.bv_val(0xAB, 8));
  s.add(low.expr() == ctx_.bv_val(0xCD, 8));
  s.add(full.expr() != ctx_.bv_val(0xABCD, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ConcatVariadic) {
  SymUInt<4> a(ctx_, "a");
  SymUInt<4> b(ctx_, "b");
  SymUInt<4> c(ctx_, "c");
  auto result = concat(a, b, c);

  static_assert(decltype(result)::kWidth == 12);
}

TEST_F(SymBitVecTest, ConcatWithSymBool) {
  SymBool flag(ctx_, "flag");
  SymUInt<7> val(ctx_, "val");
  auto result = concat(flag, val);

  static_assert(decltype(result)::kWidth == 8);
  static_assert(!decltype(result)::kIsSigned);

  // flag=true (1), val=0x55 (1010101) -> 0xD5 (11010101)
  z3::solver s(ctx_);
  s.add(flag.expr());
  s.add(val.expr() == ctx_.bv_val(0x55, 7));
  s.add(result.expr() != ctx_.bv_val(0xD5, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- shl ---

TEST_F(SymBitVecTest, ShlConstant) {
  SymUInt<8> a(ctx_, "a");
  auto result = shl<3>(a);

  static_assert(decltype(result)::kWidth == 11);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xFF, 8));
  // 0xFF << 3 = 0x7F8.
  s.add(result.expr() != ctx_.bv_val(0x7F8, 11));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShlSymbolic) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<3> n(ctx_, "n");
  auto result = shl(a, n);

  // Result width = 8 + 2^3 - 1 = 15.
  static_assert(decltype(result)::kWidth == 15);
}

TEST_F(SymBitVecTest, ShlWithTruncation) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  // Same-width left shift via shl + unsafe_cast.
  auto wide = shl(a, n);
  auto result = unsafe_cast<SymUInt<8>>(wide);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(1, 8));
  s.add(n.expr() == ctx_.bv_val(4, 8));
  s.add(result.expr() != ctx_.bv_val(16, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- shr ---

TEST_F(SymBitVecTest, ShrUnsigned) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  auto result = shr(a, n);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));
  s.add(n.expr() == ctx_.bv_val(4, 8));
  // Logical shift: 0x80 >> 4 = 0x08.
  s.add(result.expr() != ctx_.bv_val(0x08, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShrSigned) {
  SymSInt<8> a(ctx_, "a");
  SymUInt<8> n(ctx_, "n");
  auto result = shr(a, n);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));  // -128 signed
  s.add(n.expr() == ctx_.bv_val(4, 8));
  // Arithmetic shift: 0x80 >> 4 = 0xF8 (sign-extended).
  s.add(result.expr() != ctx_.bv_val(0xF8, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShrConstantUnsigned) {
  SymUInt<8> a(ctx_, "a");
  auto result = shr<3>(a);
  static_assert(std::is_same_v<decltype(result), SymUInt<8>>);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));
  // Logical shift: 0x80 >> 3 = 0x10.
  s.add(result.expr() != ctx_.bv_val(0x10, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShrConstantSigned) {
  SymSInt<8> a(ctx_, "a");
  auto result = shr<3>(a);
  static_assert((std::is_same_v<decltype(result), SymSInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));  // -128 signed
  // Arithmetic shift: 0x80 >> 3 = 0xF0 (sign-extended).
  s.add(result.expr() != ctx_.bv_val(0xF0, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShrDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<3> n(ctx_, "n");
  auto result = shr(a, n);
  static_assert(std::is_same_v<decltype(result), SymUInt<8>>);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));
  s.add(n.expr() == ctx_.bv_val(4, 3));
  // Logical shift: 0x80 >> 4 = 0x08.
  s.add(result.expr() != ctx_.bv_val(0x08, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShlConcreteAmount) {
  SymUInt<8> a(ctx_, "a");
  auto one = UInt<3>::Literal<1>();
  auto result = shl(a, one);

  // Result width = 8 + 2^3 - 1 = 15, same as symbolic SymUInt<3> amount.
  static_assert(decltype(result)::kWidth == 15);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xFF, 8));
  // 0xFF << 1 = 0x1FE.
  s.add(result.expr() != ctx_.bv_val(0x1FE, 15));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShrConcreteAmount) {
  SymUInt<8> a(ctx_, "a");
  auto one = UInt<8>::Literal<4>();
  auto result = shr(a, one);

  static_assert((std::is_same_v<decltype(result), SymUInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));
  // Logical shift: 0x80 >> 4 = 0x08.
  s.add(result.expr() != ctx_.bv_val(0x08, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ShrSignedWithUnsignedAmount) {
  SymSInt<8> a(ctx_, "a");
  SymUInt<3> n(ctx_, "n");
  auto result = shr(a, n);
  static_assert((std::is_same_v<decltype(result), SymSInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));  // -128 signed
  s.add(n.expr() == ctx_.bv_val(4, 3));
  // Arithmetic shift: 0x80 >> 4 = 0xF8 (sign-extended).
  s.add(result.expr() != ctx_.bv_val(0xF8, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- rotl ---

TEST_F(SymBitVecTest, ConstantRotl) {
  SymUInt<8> a(ctx_, "a");
  auto result = rotl<3>(a);

  static_assert((std::is_same_v<decltype(result), SymUInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xB2, 8));
  // rotl(0xB2, 3) = 0xB2 << 3 | 0xB2 >> 5 = 0x90 | 0x05 = 0x95.
  s.add(result.expr() != ctx_.bv_val(0x95, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SymbolicRotl) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<3> n(ctx_, "n");
  auto result = rotl(a, n);

  static_assert((std::is_same_v<decltype(result), SymUInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xB2, 8));
  s.add(n.expr() == ctx_.bv_val(3, 3));
  s.add(result.expr() != ctx_.bv_val(0x95, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ConcreteRotl) {
  SymUInt<8> a(ctx_, "a");
  auto n = UInt<3>::From(3);
  auto result = rotl(a, n);

  static_assert((std::is_same_v<decltype(result), SymUInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xB2, 8));
  s.add(result.expr() != ctx_.bv_val(0x95, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ConstantRotlPreservesSignedness) {
  SymSInt<8> a(ctx_, "a");
  auto result = rotl<3>(a);

  static_assert((std::is_same_v<decltype(result), SymSInt<8>>));
}

// --- rotr ---

TEST_F(SymBitVecTest, ConstantRotr) {
  SymUInt<8> a(ctx_, "a");
  auto result = rotr<3>(a);

  static_assert((std::is_same_v<decltype(result), SymUInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xB2, 8));
  // rotr(0xB2, 3) = 0xB2 >> 3 | 0xB2 << 5 = 0x16 | 0x40 = 0x56.
  s.add(result.expr() != ctx_.bv_val(0x56, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SymbolicRotr) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<3> n(ctx_, "n");
  auto result = rotr(a, n);

  static_assert((std::is_same_v<decltype(result), SymUInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xB2, 8));
  s.add(n.expr() == ctx_.bv_val(3, 3));
  s.add(result.expr() != ctx_.bv_val(0x56, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ConcreteRotr) {
  SymUInt<8> a(ctx_, "a");
  auto n = UInt<3>::From(3);
  auto result = rotr(a, n);

  static_assert((std::is_same_v<decltype(result), SymUInt<8>>));

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0xB2, 8));
  s.add(result.expr() != ctx_.bv_val(0x56, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, ConstantRotrPreservesSignedness) {
  SymSInt<8> a(ctx_, "a");
  auto result = rotr<3>(a);

  static_assert((std::is_same_v<decltype(result), SymSInt<8>>));
}

TEST_F(SymBitVecTest, RotlRotrRoundtrip) {
  SymUInt<8> a(ctx_, "a");
  // rotl then rotr by same amount should be identity.
  auto result = rotr<5>(rotl<5>(a));

  z3::solver s(ctx_);
  s.add(result.expr() != a.expr());
  EXPECT_EQ(s.check(), z3::unsat);
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
  s.add(symbolic.expr() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, SignedConcreteToSymbolic) {
  auto concrete = SInt<8>::Literal<-128>();
  SymSInt<8> symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.expr() != ctx_.bv_val(0x80, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Symbolic to concrete ---

TEST_F(SymBitVecTest, SymbolicToConcrete) {
  SymUInt<8> sym(ctx_, "x");

  z3::solver s(ctx_);
  s.add(sym.expr() == ctx_.bv_val(42, 8));
  ASSERT_EQ(s.check(), z3::sat);

  auto concrete = to_concrete(sym, s.get_model());
  EXPECT_EQ(concrete.value(), 42);
}

TEST_F(SymBitVecTest, SignedSymbolicToConcrete) {
  SymSInt<8> sym(ctx_, "x");

  z3::solver s(ctx_);
  s.add(sym.expr() == ctx_.bv_val(0x80, 8));
  ASSERT_EQ(s.check(), z3::sat);

  auto concrete = to_concrete(sym, s.get_model());
  EXPECT_EQ(concrete.value(), -128);
}

TEST_F(SymBitVecTest, SymbolicToConcreteRoundTrip) {
  auto original = UInt<8>::Literal<255>();
  SymUInt<8> sym = to_symbolic(original, ctx_);

  z3::solver s(ctx_);
  ASSERT_EQ(s.check(), z3::sat);

  auto result = to_concrete(sym, s.get_model());
  EXPECT_EQ(result.value(), original.value());
}

TEST_F(SymBitVecTest, SymbolicToConcreteMaxUInt64) {
  SymUInt<64> sym(ctx_, "x");

  z3::solver s(ctx_);
  // UINT64_MAX = 0xFFFFFFFFFFFFFFFF
  s.add(sym.expr() == ctx_.bv_val(UINT64_MAX, 64));
  ASSERT_EQ(s.check(), z3::sat);

  auto concrete = to_concrete(sym, s.get_model());
  EXPECT_EQ(concrete.value(), UINT64_MAX);
}

// --- Mixed concrete + symbolic arithmetic ---

TEST_F(SymBitVecTest, MixedAddSymbolicPlusConcrete) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<42>();
  auto result = sym + conc;

  static_assert(decltype(result)::kWidth == 9);
  static_assert(is_symbolic_v<decltype(result)>);

  z3::solver s(ctx_);
  s.add(sym.expr() == ctx_.bv_val(10, 8));
  s.add(result.expr() != ctx_.bv_val(52, 9));
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

TEST_F(SymBitVecTest, MixedMulSymbolicTimesConcrete) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<15>();
  auto result = sym * conc;

  static_assert(decltype(result)::kWidth == 16);
  static_assert(is_symbolic_v<decltype(result)>);

  z3::solver s(ctx_);
  s.add(sym.expr() == ctx_.bv_val(15, 8));
  s.add(result.expr() != ctx_.bv_val(225, 16));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedMulConcreteTimesSymbolic) {
  auto conc = UInt<8>::Literal<15>();
  SymUInt<8> sym(ctx_, "x");
  auto result = conc * sym;

  static_assert(decltype(result)::kWidth == 16);
  static_assert(is_symbolic_v<decltype(result)>);
}

// --- Mixed bitwise ---

TEST_F(SymBitVecTest, MixedBitwiseAnd) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<0x0F>();
  SymUInt<8> result = sym & conc;

  z3::solver s(ctx_);
  s.add(sym.expr() == ctx_.bv_val(0xAB, 8));
  s.add(result.expr() != ctx_.bv_val(0x0B, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed comparison ---

TEST_F(SymBitVecTest, MixedEquality) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<42>();
  SymBool eq = (sym == conc);

  z3::solver s(ctx_);
  s.add(eq.expr());
  s.add(sym.expr() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedLessThan) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<100>();
  SymBool lt = (sym < conc);

  z3::solver s(ctx_);
  s.add(lt.expr());
  s.add(sym.expr() == ctx_.bv_val(200, 8));
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
  s.add(cond.expr());
  s.add(result.expr() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedIteSymbolicCondMixedValues) {
  SymBool cond(ctx_, "c");
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<8>::Literal<99>();
  auto result = ite(cond, sym, conc);

  static_assert(is_symbolic_v<decltype(result)>);
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
  s.add(a.expr() == ctx_.bv_val(42, 8));
  s.add(b.expr() == ctx_.bv_val(42, 16));
  s.add(!eq.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CrossTypeInequalityDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<16> b(ctx_, "b");
  SymBool neq = (a != b);

  // If a=42 and b=42, they should not be unequal.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(42, 8));
  s.add(b.expr() == ctx_.bv_val(42, 16));
  s.add(neq.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, CrossTypeLessThanDifferentWidths) {
  SymUInt<8> a(ctx_, "a");
  SymUInt<16> b(ctx_, "b");
  SymBool lt = (a < b);

  // 100 < 200 should be sat (unsigned).
  z3::solver s(ctx_);
  s.add(lt.expr());
  s.add(a.expr() == ctx_.bv_val(100, 8));
  s.add(b.expr() == ctx_.bv_val(200, 16));
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
  s.add(lt.expr());
  s.add(a.expr() == ctx_.bv_val(0xFF, 8));  // -1 signed
  s.add(b.expr() == ctx_.bv_val(200, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(SymBitVecTest, CrossTypeGreaterEqualDifferentSignedness) {
  // SymUInt<8> 200 >= SymSInt<8> -1 should be true.
  SymUInt<8> a(ctx_, "a");
  SymSInt<8> b(ctx_, "b");
  SymBool ge = (a >= b);

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(200, 8));
  s.add(b.expr() == ctx_.bv_val(0xFF, 8));  // -1 signed
  s.add(!ge.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed concrete+symbolic cross-type comparison ---

TEST_F(SymBitVecTest, MixedCrossTypeEquality) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<16>::Literal<42>();
  SymBool eq = (sym == conc);

  z3::solver s(ctx_);
  s.add(eq.expr());
  s.add(sym.expr() != ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, MixedCrossTypeLessThan) {
  SymUInt<8> sym(ctx_, "x");
  auto conc = UInt<16>::Literal<300>();
  SymBool lt = (sym < conc);

  // Any 8-bit unsigned value (0-255) is always < 300.
  z3::solver s(ctx_);
  s.add(!lt.expr());
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
  s.add(a.expr() == ctx_.bv_val(100, 8));
  s.add(neg.expr() != ctx_.bv_val(412, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, NegateSigned) {
  SymSInt<8> a(ctx_, "a");
  auto neg = -a;

  static_assert(decltype(neg)::kWidth == 9);
  static_assert(decltype(neg)::kIsSigned);

  // Negate -128 -> 128, which fits in SymSInt<9>.
  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0x80, 8));  // -128 signed
  s.add(neg.expr() != ctx_.bv_val(128, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBitVecTest, NegateZero) {
  SymUInt<8> a(ctx_, "a");
  auto neg = -a;

  z3::solver s(ctx_);
  s.add(a.expr() == ctx_.bv_val(0, 8));
  s.add(neg.expr() != ctx_.bv_val(0, 9));
  EXPECT_EQ(s.check(), z3::unsat);
}

// --- Mixed unary negate ---

TEST_F(SymBitVecTest, NegateConsistentWithBinarySub) {
  SymUInt<8> a(ctx_, "a");
  auto neg = -a;
  auto zero_minus_a = SymUInt<8>::Literal<0>(ctx_) - a;

  // -a should equal 0 - a for all values.
  z3::solver s(ctx_);
  s.add(neg.expr() != zero_minus_a.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST(SymBitVecDeathTest, ExprOnUninitializedAborts) {
  SymUInt<8> uninit;
  EXPECT_DEATH((void)uninit.expr(), "SymBitVec used before initialization");
}

// --- Wide concrete to symbolic (W > 64) ---

TEST_F(SymBitVecTest, WideToSymbolic) {
  auto concrete = UInt<128>::Literal<42>();
  auto symbolic = to_symbolic(concrete, ctx_);
  EXPECT_EQ(symbolic.expr().get_sort().bv_size(), 128);

  // Verify the value via solver.
  z3::solver s(ctx_);
  s.add(symbolic.expr() == ctx_.bv_val(uint64_t{42}, 128));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(SymBitVecTest, WideToSymbolicZero) {
  UInt<128> concrete;
  auto symbolic = to_symbolic(concrete, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.expr() == ctx_.bv_val(uint64_t{0}, 128));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(SymBitVecTest, WideToSymbolicLargeValue) {
  // Construct a 128-bit value with bytes set in high positions.
  std::array<uint8_t, 16> bytes{};
  bytes[0] = 0xEF;
  bytes[8] = 0xAB;
  auto r = UInt<128>::TryFrom(bytes);
  auto& concrete = r.value;
  auto truncated = r.truncated;
  ASSERT_FALSE(truncated);

  auto symbolic = to_symbolic(concrete, ctx_);

  // Verify by extracting bits.
  z3::solver s(ctx_);
  s.add(symbolic.expr().extract(7, 0) == ctx_.bv_val(0xEF, 8));
  s.add(symbolic.expr().extract(71, 64) == ctx_.bv_val(0xAB, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

// --- Wide symbolic to concrete (W > 64) ---

TEST_F(SymBitVecTest, WideToConcrete) {
  SymUInt<128> x(ctx_, "x");

  z3::solver s(ctx_);
  s.add(x.expr() == ctx_.bv_val(uint64_t{42}, 128));
  ASSERT_EQ(s.check(), z3::sat);

  auto concrete = to_concrete(x, s.get_model());
  auto bytes = concrete.value();
  EXPECT_EQ(bytes[0], 42);
  for (size_t i = 1; i < bytes.size(); ++i) {
    EXPECT_EQ(bytes[i], 0);
  }
}

TEST_F(SymBitVecTest, WideToConcreteRoundTrip) {
  // Construct a wide concrete value, convert to symbolic, solve, convert back.
  std::array<uint8_t, 16> bytes{};
  bytes[0] = 0xEF;
  bytes[8] = 0xAB;
  auto r = UInt<128>::TryFrom(bytes);
  auto& original = r.value;
  auto truncated = r.truncated;
  ASSERT_FALSE(truncated);

  auto symbolic = to_symbolic(original, ctx_);

  z3::solver s(ctx_);
  s.add(symbolic.expr() == symbolic.expr());  // trivially sat
  ASSERT_EQ(s.check(), z3::sat);

  auto result = to_concrete(symbolic, s.get_model());
  EXPECT_EQ(result.value(), original.value());
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST(SymBitVecDeathTest, ConstructorRejectsWrongSort) {
  z3::context ctx;
  z3::expr bool_expr = ctx.bool_val(true);
  EXPECT_DEATH((void)SymUInt<8>(bool_expr), "");
}

TEST(SymBitVecDeathTest, ConstructorRejectsWrongWidth) {
  z3::context ctx;
  z3::expr bv16 = ctx.bv_val(0, 16);
  EXPECT_DEATH((void)SymUInt<8>(bv16), "");
}
// NOLINTEND(readability-function-cognitive-complexity)

class FromExprTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

TEST_F(FromExprTest, SymBitVecFromExprWithValidExpr) {
  auto result = SymUInt<8>::FromExpr(ctx_.bv_val(42, 8));
  ASSERT_TRUE(result.has_value());
  z3::solver s(ctx_);
  s.add(result->expr() == ctx_.bv_val(42, 8));
  EXPECT_EQ(s.check(), z3::sat);
}

TEST_F(FromExprTest, SymBitVecFromExprWithWrongWidth) {
  auto result = SymUInt<8>::FromExpr(ctx_.bv_val(0, 16));
  EXPECT_FALSE(result.has_value());
}

TEST_F(FromExprTest, SymBitVecFromExprWithBoolExpr) {
  auto result = SymUInt<8>::FromExpr(ctx_.bool_val(true));
  EXPECT_FALSE(result.has_value());
}

}  // namespace
}  // namespace z3w
