#include "z3wire/int.h"

#include <gtest/gtest.h>

#include <cstdint>

namespace z3w {
namespace {

// --- Storage type ---

TEST(IntTest, StorageType) {
  static_assert(std::is_same_v<StorageType<1>, uint8_t>);
  static_assert(std::is_same_v<StorageType<8>, uint8_t>);
  static_assert(std::is_same_v<StorageType<9>, uint16_t>);
  static_assert(std::is_same_v<StorageType<16>, uint16_t>);
  static_assert(std::is_same_v<StorageType<17>, uint32_t>);
  static_assert(std::is_same_v<StorageType<32>, uint32_t>);
  static_assert(std::is_same_v<StorageType<33>, uint64_t>);
  static_assert(std::is_same_v<StorageType<64>, uint64_t>);
}

// --- Type traits ---

TEST(IntTest, TypeTraits) {
  static_assert(UInt<8>::kWidth == 8);
  static_assert(!UInt<8>::kIsSigned);
  static_assert(SInt<8>::kWidth == 8);
  static_assert(SInt<8>::kIsSigned);
}

TEST(IntTest, IsConcreteV) {
  static_assert(is_concrete_v<UInt<8>>);
  static_assert(is_concrete_v<SInt<16>>);
  static_assert(!is_concrete_v<int>);
}

// --- Raw constructor (masking) ---

TEST(IntTest, RawConstructorMasks) {
  UInt<8> a(300);
  EXPECT_EQ(a.value(), 44);  // 300 & 0xFF
}

TEST(IntTest, RawConstructorNonPowerOfTwo) {
  UInt<5> a(0xFF);
  EXPECT_EQ(a.value(), 0x1F);  // 0xFF & 0x1F
}

TEST(IntTest, RawConstructorSigned) {
  SInt<8> a(300);
  EXPECT_EQ(a.value(), 44);  // Same underlying bits
}

// --- Literal (compile-time checked) ---

TEST(IntTest, LiteralUnsigned) {
  auto a = UInt<8>::Literal<255>();
  EXPECT_EQ(a.value(), 255);
}

TEST(IntTest, LiteralZero) {
  auto a = UInt<8>::Literal<0>();
  EXPECT_EQ(a.value(), 0);
}

TEST(IntTest, LiteralSigned) {
  // SInt<8> range: -128 to 127.
  auto a = SInt<8>::Literal<127>();
  EXPECT_EQ(a.value(), 127);
}

TEST(IntTest, LiteralSignedNegative) {
  auto a = SInt<8>::Literal<-128>();
  EXPECT_EQ(a.value(), 0x80);  // -128 in 8-bit two's complement
  auto b = SInt<8>::Literal<-1>();
  EXPECT_EQ(b.value(), 0xFF);  // -1 in 8-bit two's complement
}

// --- Checked construction ---

TEST(IntTest, CheckedNoTruncation) {
  auto [val, truncated] = UInt<8>::checked(200);
  EXPECT_EQ(val.value(), 200);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, CheckedWithTruncation) {
  auto [val, truncated] = UInt<8>::checked(300);
  EXPECT_EQ(val.value(), 44);
  EXPECT_TRUE(truncated);
}

// --- Bitwise operators ---

TEST(IntTest, BitwiseAnd) {
  UInt<8> a(0xF0);
  UInt<8> b(0x3C);
  UInt<8> c = a & b;
  EXPECT_EQ(c.value(), 0x30);
}

TEST(IntTest, BitwiseOr) {
  UInt<8> a(0xF0);
  UInt<8> b(0x0F);
  UInt<8> c = a | b;
  EXPECT_EQ(c.value(), 0xFF);
}

TEST(IntTest, BitwiseXor) {
  UInt<8> a(0xFF);
  UInt<8> b(0x0F);
  UInt<8> c = a ^ b;
  EXPECT_EQ(c.value(), 0xF0);
}

TEST(IntTest, BitwiseNot) {
  UInt<8> a(0xF0);
  UInt<8> b = ~a;
  EXPECT_EQ(b.value(), 0x0F);
}

TEST(IntTest, BitwiseNotNonPowerOfTwo) {
  UInt<5> a(0x1F);
  UInt<5> b = ~a;
  EXPECT_EQ(b.value(), 0x00);  // ~0x1F masked to 5 bits
}

// --- Equality ---

TEST(IntTest, Equality) {
  UInt<8> a(42);
  UInt<8> b(42);
  UInt<8> c(99);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(IntTest, Inequality) {
  UInt<8> a(42);
  UInt<8> b(99);
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a != a);
}

// --- Ordered comparison (unsigned) ---

TEST(IntTest, UnsignedLessThan) {
  UInt<8> a(100);
  UInt<8> b(200);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

TEST(IntTest, UnsignedGreaterThan) {
  UInt<8> a(200);
  UInt<8> b(100);
  EXPECT_TRUE(a > b);
  EXPECT_FALSE(b > a);
}

TEST(IntTest, UnsignedLessEqual) {
  UInt<8> a(100);
  UInt<8> b(100);
  EXPECT_TRUE(a <= b);
}

TEST(IntTest, UnsignedGreaterEqual) {
  UInt<8> a(100);
  UInt<8> b(100);
  EXPECT_TRUE(a >= b);
}

// --- Ordered comparison (signed) ---

TEST(IntTest, SignedLessThan) {
  // 200 as SInt<8> is -56. So -56 < 100.
  SInt<8> a(200);  // -56
  SInt<8> b(100);
  EXPECT_TRUE(a < b);
}

TEST(IntTest, SignedGreaterThan) {
  SInt<8> a(100);
  SInt<8> b(200);  // -56
  EXPECT_TRUE(a > b);
}

TEST(IntTest, SignedComparisonNonPowerOfTwo) {
  // SInt<5>: range -16 to 15. Value 0x1F = 31 unsigned = -1 signed.
  SInt<5> a(0x1F);  // -1
  SInt<5> b(1);
  EXPECT_TRUE(a < b);
}

// --- Arithmetic with bit growth ---

TEST(IntTest, AdditionWidens) {
  UInt<8> a(255);
  UInt<8> b(255);
  auto sum = a + b;
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(!decltype(sum)::kIsSigned);
  EXPECT_EQ(sum.value(), 510);
}

TEST(IntTest, AdditionDifferentWidths) {
  UInt<8> a(255);
  UInt<4> b(15);
  auto sum = a + b;
  static_assert(decltype(sum)::kWidth == 9);
  EXPECT_EQ(sum.value(), 270);
}

TEST(IntTest, AdditionMixedSignedness) {
  UInt<8> a(100);
  SInt<8> b(200);  // -56 signed
  auto sum = a + b;
  static_assert(decltype(sum)::kWidth == 9);
  static_assert(decltype(sum)::kIsSigned);
  // 100 + (-56) = 44
  using SInt9 = Int<9, true>;
  EXPECT_EQ(SInt9::Literal<44>().value(), sum.value());
}

TEST(IntTest, SubtractionIsSigned) {
  UInt<8> a(100);
  UInt<8> b(200);
  auto diff = a - b;
  static_assert(decltype(diff)::kWidth == 9);
  static_assert(decltype(diff)::kIsSigned);
  // 100 - 200 = -100. In 9-bit two's complement: 512 - 100 = 412.
  EXPECT_EQ(diff.value(), 412);
}

TEST(IntTest, SubtractionPositiveResult) {
  UInt<8> a(200);
  UInt<8> b(100);
  auto diff = a - b;
  EXPECT_EQ(diff.value(), 100);
}

// --- Hardware shifts ---

TEST(IntTest, LeftShift) {
  UInt<8> a(1);
  UInt<8> n(4);
  UInt<8> result = a << n;
  EXPECT_EQ(result.value(), 16);
}

TEST(IntTest, LeftShiftOverflow) {
  UInt<8> a(0x80);
  UInt<8> n(1);
  UInt<8> result = a << n;
  EXPECT_EQ(result.value(), 0);  // Bit shifted out
}

TEST(IntTest, UnsignedRightShift) {
  UInt<8> a(0x80);
  UInt<8> n(4);
  UInt<8> result = a >> n;
  EXPECT_EQ(result.value(), 0x08);  // Logical shift
}

TEST(IntTest, SignedRightShift) {
  SInt<8> a(0x80);  // -128
  SInt<8> n(4);
  SInt<8> result = a >> n;
  EXPECT_EQ(result.value(), 0xF8);  // Arithmetic shift: sign-extended
}

// --- Checked shifts ---

TEST(IntTest, CheckedShlNoLoss) {
  UInt<8> a(1);
  UInt<8> n(4);
  auto [shifted, lost] = checked_shl(a, n);
  EXPECT_EQ(shifted.value(), 16);
  EXPECT_FALSE(lost);
}

TEST(IntTest, CheckedShlWithLoss) {
  UInt<8> a(0x80);
  UInt<8> n(1);
  auto [shifted, lost] = checked_shl(a, n);
  EXPECT_EQ(shifted.value(), 0);
  EXPECT_TRUE(lost);
}

TEST(IntTest, CheckedShrWithLoss) {
  UInt<8> a(1);
  UInt<8> n(1);
  auto [shifted, lost] = checked_shr(a, n);
  EXPECT_EQ(shifted.value(), 0);
  EXPECT_TRUE(lost);
}

// --- Lossless left shift ---

TEST(IntTest, LosslessShlConstant) {
  UInt<8> a(0xFF);
  auto result = lossless_shl<3>(a);
  static_assert(decltype(result)::kWidth == 11);
  EXPECT_EQ(result.value(), 0x7F8);
}

// --- cast ---

TEST(IntTest, CastTruncation) {
  UInt<16> a(0x1234);
  auto b = cast<UInt<8>>(a);
  static_assert(decltype(b)::kWidth == 8);
  EXPECT_EQ(b.value(), 0x34);
}

TEST(IntTest, CastZeroExtension) {
  UInt<8> a(0xFF);
  auto b = cast<UInt<16>>(a);
  static_assert(decltype(b)::kWidth == 16);
  EXPECT_EQ(b.value(), 0x00FF);
}

TEST(IntTest, CastSignExtension) {
  SInt<8> a(0x80);  // -128
  auto b = cast<SInt<16>>(a);
  EXPECT_EQ(b.value(), 0xFF80);
}

TEST(IntTest, CastBitcast) {
  UInt<8> a(42);
  auto b = cast<SInt<8>>(a);
  static_assert(decltype(b)::kIsSigned);
  EXPECT_EQ(b.value(), 42);
}

// --- safe_cast ---

TEST(IntTest, SafeCastWidening) {
  UInt<8> a(255);
  auto b = safe_cast<UInt<16>>(a);
  static_assert(decltype(b)::kWidth == 16);
  EXPECT_EQ(b.value(), 255);
}

TEST(IntTest, SafeCastUnsignedToSigned) {
  UInt<8> a(255);
  auto b = safe_cast<SInt<9>>(a);
  static_assert(decltype(b)::kWidth == 9);
  EXPECT_EQ(b.value(), 255);
}

// --- checked_cast ---

TEST(IntTest, CheckedCastNoOverflow) {
  UInt<16> a(42);
  auto [result, overflowed] = checked_cast<UInt<8>>(a);
  EXPECT_EQ(result.value(), 42);
  EXPECT_FALSE(overflowed);
}

TEST(IntTest, CheckedCastWithOverflow) {
  UInt<16> a(256);
  auto [result, overflowed] = checked_cast<UInt<8>>(a);
  EXPECT_EQ(result.value(), 0);
  EXPECT_TRUE(overflowed);
}

// --- extract ---

TEST(IntTest, StaticExtract) {
  UInt<16> a(0x1234);
  auto high = extract<15, 8>(a);
  auto low = extract<7, 0>(a);
  static_assert(decltype(high)::kWidth == 8);
  static_assert(decltype(low)::kWidth == 8);
  EXPECT_EQ(high.value(), 0x12);
  EXPECT_EQ(low.value(), 0x34);
}

// --- concat ---

TEST(IntTest, Concat) {
  UInt<8> high(0xAB);
  UInt<8> low(0xCD);
  auto full = concat(high, low);
  static_assert(decltype(full)::kWidth == 16);
  EXPECT_EQ(full.value(), 0xABCD);
}

TEST(IntTest, ConcatVariadic) {
  UInt<4> a(0xA);
  UInt<4> b(0xB);
  UInt<4> c(0xC);
  auto result = concat(a, b, c);
  static_assert(decltype(result)::kWidth == 12);
  EXPECT_EQ(result.value(), 0xABC);
}

// --- Bool / UInt<1> conversion ---

TEST(IntTest, ToUInt1) {
  auto one = to_uint1(true);
  auto zero = to_uint1(false);
  EXPECT_EQ(one.value(), 1);
  EXPECT_EQ(zero.value(), 0);
}

TEST(IntTest, ToBoolFromUInt1) {
  UInt<1> one(1);
  UInt<1> zero(0);
  EXPECT_TRUE(to_bool(one));
  EXPECT_FALSE(to_bool(zero));
}

// --- ite ---

TEST(IntTest, IteTrue) {
  UInt<8> a(42);
  UInt<8> b(99);
  UInt<8> result = ite(true, a, b);
  EXPECT_EQ(result.value(), 42);
}

TEST(IntTest, IteFalse) {
  UInt<8> a(42);
  UInt<8> b(99);
  UInt<8> result = ite(false, a, b);
  EXPECT_EQ(result.value(), 99);
}

// --- W=64 boundary ---

TEST(IntTest, Width64Construction) {
  UInt<64> a(UINT64_MAX);
  EXPECT_EQ(a.value(), UINT64_MAX);
  auto [val, truncated] = UInt<64>::checked(UINT64_MAX);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, Width64Arithmetic) {
  UInt<63> a(UINT64_MAX >> 1);  // max 63-bit value
  UInt<63> b(1);
  auto sum = a + b;
  static_assert(decltype(sum)::kWidth == 64);
  // (2^63 - 1) + 1 = 2^63
  EXPECT_EQ(sum.value(), uint64_t{1} << 63);
}

TEST(IntTest, Width64SignedComparison) {
  // SInt<64>: -1 should be less than 1
  SInt<64> neg(UINT64_MAX);  // -1 in two's complement
  SInt<64> pos(1);
  EXPECT_TRUE(neg < pos);
  EXPECT_FALSE(pos < neg);
}

TEST(IntTest, Width1) {
  UInt<1> a(3);
  EXPECT_EQ(a.value(), 1);  // 3 & 0x1
  UInt<1> b(0);
  EXPECT_TRUE(a > b);
}

// --- Signed checked construction ---

TEST(IntTest, SignedCheckedNoTruncation) {
  auto [val, truncated] = SInt<8>::checked(int64_t{-128});
  EXPECT_EQ(val.value(), 0x80);  // -128 in two's complement
  EXPECT_FALSE(truncated);
}

TEST(IntTest, SignedCheckedPositive) {
  auto [val, truncated] = SInt<8>::checked(int64_t{127});
  EXPECT_EQ(val.value(), 127);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, SignedCheckedOverflowNegative) {
  auto [val, truncated] = SInt<8>::checked(int64_t{-200});
  EXPECT_TRUE(truncated);
}

TEST(IntTest, SignedCheckedOverflowPositive) {
  auto [val, truncated] = SInt<8>::checked(int64_t{200});
  EXPECT_TRUE(truncated);
}

// --- Cross-type comparison (different widths and/or signedness) ---

TEST(IntTest, CrossTypeEqualitySameSignedness) {
  // UInt<8>(42) == UInt<16>(42) → true
  UInt<8> a(42);
  UInt<16> b(42);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(IntTest, CrossTypeEqualityDifferentValues) {
  UInt<8> a(100);
  UInt<16> b(200);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(IntTest, CrossTypeLessThanDifferentWidths) {
  // UInt<8>(100) < UInt<16>(200) → true
  UInt<8> a(100);
  UInt<16> b(200);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_TRUE(a <= b);
  EXPECT_FALSE(b <= a);
  EXPECT_TRUE(b > a);
  EXPECT_FALSE(a > b);
  EXPECT_TRUE(b >= a);
  EXPECT_FALSE(a >= b);
}

TEST(IntTest, CrossTypeDifferentSignednessSameWidth) {
  // SInt<8>(0xFF) is -1 signed, UInt<8>(1) is 1 unsigned.
  // Common type: signed, width = max(8,8)+1 = 9.
  // -1 < 1 → true
  SInt<8> a(0xFF);  // -1
  UInt<8> b(1);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(IntTest, CrossTypeDifferentSignednessAndWidth) {
  // SInt<8>(0x80) is -128 signed, UInt<16>(200) is 200 unsigned.
  // Common type: signed, width = max(8,16)+1 = 17.
  // -128 < 200 → true
  SInt<8> a(0x80);  // -128
  UInt<16> b(200);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

TEST(IntTest, CrossTypeUnsignedNeedsExtraSignBit) {
  // UInt<8>(255) compared with SInt<8>(-1).
  // Common type: signed, width = max(8,8)+1 = 9.
  // UInt 255 zero-extends to 9 bits → 255 (positive).
  // SInt -1 sign-extends to 9 bits → -1.
  // 255 > -1 → true
  UInt<8> a(255);
  SInt<8> b(0xFF);  // -1
  EXPECT_TRUE(a > b);
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(a == b);
}

TEST(IntTest, CrossTypeEqualityMixedSignedness) {
  // UInt<8>(42) == SInt<16>(42) → true
  // Common type: signed, width = max(8,16)+1 = 17.
  UInt<8> a(42);
  SInt<16> b(42);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

}  // namespace
}  // namespace z3w
