#include "z3wire/int.h"

#include <cstdint>

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

// --- W=64 boundary ---

TEST(IntTest, Width64Construction) {
  UInt<64> a(UINT64_MAX);
  EXPECT_EQ(a.value(), UINT64_MAX);
  auto [val, truncated] = UInt<64>::checked(UINT64_MAX);
  EXPECT_FALSE(truncated);
}

TEST(IntTest, Width1) {
  UInt<1> a(3);
  EXPECT_EQ(a.value(), 1);  // 3 & 0x1
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

// --- Cross-type equality (different widths and/or signedness) ---

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
