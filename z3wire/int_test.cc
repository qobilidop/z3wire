#include "z3wire/int.h"

#include <cstdint>

#include <gtest/gtest.h>

namespace z3w {
namespace {

// --- Storage types ---

TEST(IntTest, UnsignedStorageType) {
  static_assert(std::is_same_v<UnsignedStorageType<1>, uint8_t>);
  static_assert(std::is_same_v<UnsignedStorageType<8>, uint8_t>);
  static_assert(std::is_same_v<UnsignedStorageType<9>, uint16_t>);
  static_assert(std::is_same_v<UnsignedStorageType<16>, uint16_t>);
  static_assert(std::is_same_v<UnsignedStorageType<17>, uint32_t>);
  static_assert(std::is_same_v<UnsignedStorageType<32>, uint32_t>);
  static_assert(std::is_same_v<UnsignedStorageType<33>, uint64_t>);
  static_assert(std::is_same_v<UnsignedStorageType<64>, uint64_t>);
}

TEST(IntTest, SignedStorageType) {
  static_assert(std::is_same_v<SignedStorageType<1>, int8_t>);
  static_assert(std::is_same_v<SignedStorageType<8>, int8_t>);
  static_assert(std::is_same_v<SignedStorageType<9>, int16_t>);
  static_assert(std::is_same_v<SignedStorageType<16>, int16_t>);
  static_assert(std::is_same_v<SignedStorageType<17>, int32_t>);
  static_assert(std::is_same_v<SignedStorageType<32>, int32_t>);
  static_assert(std::is_same_v<SignedStorageType<33>, int64_t>);
  static_assert(std::is_same_v<SignedStorageType<64>, int64_t>);
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
  EXPECT_EQ(a.bits(), 0x80);   // Bit pattern
  EXPECT_EQ(a.value(), -128);  // Interpreted value
  auto b = SInt<8>::Literal<-1>();
  EXPECT_EQ(b.bits(), 0xFF);  // Bit pattern
  EXPECT_EQ(b.value(), -1);   // Interpreted value
}

// --- Checked construction ---

TEST(IntTest, CheckedNoTruncation) {
  auto [val, truncated] = UInt<8>::checked(200);
  EXPECT_EQ(val.bits(), 200);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, CheckedWithTruncation) {
  auto [val, truncated] = UInt<8>::checked(300);
  EXPECT_EQ(val.bits(), 44);
  EXPECT_TRUE(truncated);
}

TEST(IntTest, CheckedNegativeOnUnsigned) {
  auto [val, truncated] = UInt<8>::checked(-1);
  EXPECT_TRUE(truncated);
}

TEST(IntTest, SignedCheckedNoTruncation) {
  auto [val, truncated] = SInt<8>::checked(-128);
  EXPECT_EQ(val.bits(), 0x80);   // Bit pattern
  EXPECT_EQ(val.value(), -128);  // Interpreted value
  EXPECT_FALSE(truncated);
}

TEST(IntTest, SignedCheckedPositive) {
  auto [val, truncated] = SInt<8>::checked(127);
  EXPECT_EQ(val.value(), 127);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, SignedCheckedOverflowNegative) {
  auto [val, truncated] = SInt<8>::checked(-200);
  EXPECT_TRUE(truncated);
}

TEST(IntTest, SignedCheckedOverflowPositive) {
  auto [val, truncated] = SInt<8>::checked(200);
  EXPECT_TRUE(truncated);
}

// --- bits() vs value() ---

TEST(IntTest, UnsignedBitsAndValueMatch) {
  auto a = UInt<8>::Literal<200>();
  EXPECT_EQ(a.bits(), 200);
  EXPECT_EQ(a.value(), 200);
  static_assert(std::is_same_v<decltype(a.bits()), uint8_t>);
  static_assert(std::is_same_v<decltype(a.value()), uint8_t>);
}

TEST(IntTest, SignedBitsVsValue) {
  auto a = SInt<8>::Literal<-1>();
  EXPECT_EQ(a.bits(), 0xFF);
  EXPECT_EQ(a.value(), -1);
  static_assert(std::is_same_v<decltype(a.bits()), uint8_t>);
  static_assert(std::is_same_v<decltype(a.value()), int8_t>);
}

TEST(IntTest, SignedBitsVsValueMinMax) {
  auto min_val = SInt<8>::Literal<-128>();
  EXPECT_EQ(min_val.bits(), 0x80);
  EXPECT_EQ(min_val.value(), -128);

  auto max_val = SInt<8>::Literal<127>();
  EXPECT_EQ(max_val.bits(), 0x7F);
  EXPECT_EQ(max_val.value(), 127);
}

TEST(IntTest, SignedNonPowerOfTwoBitsVsValue) {
  // SInt<5>: range -16 to 15. Value 0x1F = 31 unsigned = -1 signed.
  auto a = SInt<5>::Literal<-1>();
  EXPECT_EQ(a.bits(), 0x1F);
  EXPECT_EQ(a.value(), -1);
}

// --- Equality ---

TEST(IntTest, Equality) {
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<8>::Literal<42>();
  auto c = UInt<8>::Literal<99>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(IntTest, Inequality) {
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<8>::Literal<99>();
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a != a);
}

// --- W=64 boundary ---

TEST(IntTest, Width64Construction) {
  auto [val, truncated] = UInt<64>::checked(UINT64_MAX);
  EXPECT_EQ(val.bits(), UINT64_MAX);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, Width1) {
  auto [val, truncated] = UInt<1>::checked(3);
  EXPECT_EQ(val.bits(), 1);  // 3 & 0x1
  EXPECT_TRUE(truncated);
}

// --- Cross-type equality (different widths and/or signedness) ---

TEST(IntTest, CrossTypeEqualitySameSignedness) {
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<16>::Literal<42>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(IntTest, CrossTypeEqualityDifferentValues) {
  auto a = UInt<8>::Literal<100>();
  auto b = UInt<16>::Literal<200>();
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(IntTest, CrossTypeEqualityMixedSignedness) {
  // UInt<8>(42) == SInt<16>(42) → true
  // Common type: signed, width = max(8,16)+1 = 17.
  auto a = UInt<8>::Literal<42>();
  auto b = SInt<16>::Literal<42>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

}  // namespace
}  // namespace z3w
