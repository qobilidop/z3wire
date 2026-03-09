#include "z3wire/int.h"

#include <gtest/gtest.h>

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

}  // namespace
}  // namespace z3w
