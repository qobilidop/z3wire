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

}  // namespace
}  // namespace z3w
