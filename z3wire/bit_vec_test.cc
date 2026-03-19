#include "z3wire/bit_vec.h"

#include <array>
#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>

#include "z3wire/type_traits.h"

namespace z3w {
namespace {

// --- ValueType ---

TEST(BitVecTest, UnsignedValueType) {
  static_assert(std::is_same_v<UInt<1>::ValueType, uint8_t>);
  static_assert(std::is_same_v<UInt<8>::ValueType, uint8_t>);
  static_assert(std::is_same_v<UInt<9>::ValueType, uint16_t>);
  static_assert(std::is_same_v<UInt<16>::ValueType, uint16_t>);
  static_assert(std::is_same_v<UInt<17>::ValueType, uint32_t>);
  static_assert(std::is_same_v<UInt<32>::ValueType, uint32_t>);
  static_assert(std::is_same_v<UInt<33>::ValueType, uint64_t>);
  static_assert(std::is_same_v<UInt<64>::ValueType, uint64_t>);
}

TEST(BitVecTest, SignedValueType) {
  static_assert(std::is_same_v<SInt<1>::ValueType, int8_t>);
  static_assert(std::is_same_v<SInt<8>::ValueType, int8_t>);
  static_assert(std::is_same_v<SInt<9>::ValueType, int16_t>);
  static_assert(std::is_same_v<SInt<16>::ValueType, int16_t>);
  static_assert(std::is_same_v<SInt<17>::ValueType, int32_t>);
  static_assert(std::is_same_v<SInt<32>::ValueType, int32_t>);
  static_assert(std::is_same_v<SInt<33>::ValueType, int64_t>);
  static_assert(std::is_same_v<SInt<64>::ValueType, int64_t>);
}

// --- Type traits ---

TEST(BitVecTest, TypeTraits) {
  static_assert(UInt<8>::kWidth == 8);
  static_assert(!UInt<8>::kIsSigned);
  static_assert(SInt<8>::kWidth == 8);
  static_assert(SInt<8>::kIsSigned);
}

TEST(BitVecTest, IsConcreteV) {
  static_assert(is_concrete_v<UInt<8>>);
  static_assert(is_concrete_v<SInt<16>>);
  static_assert(!is_concrete_v<int>);
}

// --- Literal (compile-time checked) ---

TEST(BitVecTest, LiteralUnsigned) {
  auto a = UInt<8>::Literal<255>();
  EXPECT_EQ(a.value(), 255);
}

TEST(BitVecTest, LiteralZero) {
  auto a = UInt<8>::Literal<0>();
  EXPECT_EQ(a.value(), 0);
}

TEST(BitVecTest, LiteralSigned) {
  // SInt<8> range: -128 to 127.
  auto a = SInt<8>::Literal<127>();
  EXPECT_EQ(a.value(), 127);
}

TEST(BitVecTest, LiteralSignedNegative) {
  auto a = SInt<8>::Literal<-128>();
  EXPECT_EQ(a.value(), -128);
  auto b = SInt<8>::Literal<-1>();
  EXPECT_EQ(b.value(), -1);
}

// --- Checked construction ---

TEST(BitVecTest, CheckedNoTruncation) {
  auto [val, truncated] = UInt<8>::Checked(200);
  EXPECT_EQ(val.value(), 200);
  EXPECT_FALSE(truncated);
}

TEST(BitVecTest, CheckedWithTruncation) {
  auto [val, truncated] = UInt<8>::Checked(300);
  EXPECT_EQ(val.value(), 44);
  EXPECT_TRUE(truncated);
}

TEST(BitVecTest, CheckedNegativeOnUnsigned) {
  auto [val, truncated] = UInt<8>::Checked(-1);
  EXPECT_TRUE(truncated);
}

TEST(BitVecTest, SignedCheckedNoTruncation) {
  auto [val, truncated] = SInt<8>::Checked(-128);
  EXPECT_EQ(val.value(), -128);
  EXPECT_FALSE(truncated);
}

TEST(BitVecTest, SignedCheckedPositive) {
  auto [val, truncated] = SInt<8>::Checked(127);
  EXPECT_EQ(val.value(), 127);
  EXPECT_FALSE(truncated);
}

TEST(BitVecTest, SignedCheckedOverflowNegative) {
  auto [val, truncated] = SInt<8>::Checked(-200);
  EXPECT_TRUE(truncated);
}

TEST(BitVecTest, SignedCheckedOverflowPositive) {
  auto [val, truncated] = SInt<8>::Checked(200);
  EXPECT_TRUE(truncated);
}

// --- value() ---

TEST(BitVecTest, UnsignedValueAccess) {
  auto a = UInt<8>::Literal<200>();
  EXPECT_EQ(a.value(), 200);
  static_assert(std::is_same_v<decltype(a.value()), uint8_t>);
}

TEST(BitVecTest, SignedValueAccess) {
  auto a = SInt<8>::Literal<-1>();
  EXPECT_EQ(a.value(), -1);
  static_assert(std::is_same_v<decltype(a.value()), int8_t>);
}

TEST(BitVecTest, SignedValueMinMax) {
  auto min_val = SInt<8>::Literal<-128>();
  EXPECT_EQ(min_val.value(), -128);

  auto max_val = SInt<8>::Literal<127>();
  EXPECT_EQ(max_val.value(), 127);
}

TEST(BitVecTest, SignedNonPowerOfTwoValue) {
  // SInt<5>: range -16 to 15.
  auto a = SInt<5>::Literal<-1>();
  EXPECT_EQ(a.value(), -1);
}

// --- Equality ---

TEST(BitVecTest, Equality) {
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<8>::Literal<42>();
  auto c = UInt<8>::Literal<99>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(BitVecTest, Inequality) {
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<8>::Literal<99>();
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a != a);
}

// --- W=64 boundary ---

TEST(BitVecTest, Width64Construction) {
  auto [val, truncated] = UInt<64>::Checked(UINT64_MAX);
  EXPECT_EQ(val.value(), UINT64_MAX);
  EXPECT_FALSE(truncated);
}

TEST(BitVecTest, Width1) {
  auto [val, truncated] = UInt<1>::Checked(3);
  EXPECT_EQ(val.value(), 1);  // 3 & 0x1
  EXPECT_TRUE(truncated);
}

// --- Cross-type equality (different widths and/or signedness) ---

TEST(BitVecTest, CrossTypeEqualitySameSignedness) {
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<16>::Literal<42>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(BitVecTest, CrossTypeEqualityDifferentValues) {
  auto a = UInt<8>::Literal<100>();
  auto b = UInt<16>::Literal<200>();
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(BitVecTest, CrossTypeEqualityMixedSignedness) {
  // UInt<8>(42) == SInt<16>(42) → true
  // Common type: signed, width = max(8,16)+1 = 17.
  auto a = UInt<8>::Literal<42>();
  auto b = SInt<16>::Literal<42>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(BitVecTest, CrossTypeEqualityMixedSignednessNegative) {
  // SInt<32>(-1) != UInt<32>(4294967295) — mathematically different values,
  // even though they share the same bit pattern.
  auto a = SInt<32>::Literal<-1>();
  auto b = UInt<32>::Checked(UINT32_MAX);
  EXPECT_FALSE(a == std::get<0>(b));
}

// --- Wide (W > 64) unsigned ---

TEST(BitVecTest, WideValueType) {
  // 120 bits = 15 bytes
  static_assert(std::is_same_v<UInt<120>::ValueType, std::array<uint8_t, 15>>);
  // 128 bits = 16 bytes
  static_assert(std::is_same_v<UInt<128>::ValueType, std::array<uint8_t, 16>>);
  // 100 bits = 13 bytes (non-byte-aligned)
  static_assert(std::is_same_v<UInt<100>::ValueType, std::array<uint8_t, 13>>);
}

TEST(BitVecTest, WideTypeTraits) {
  static_assert(UInt<128>::kWidth == 128);
  static_assert(!UInt<128>::kIsSigned);
}

TEST(BitVecTest, WideIsConcreteV) { static_assert(is_concrete_v<UInt<128>>); }

TEST(BitVecTest, WideDefaultConstruction) {
  UInt<128> a;
  std::array<uint8_t, 16> expected{};
  EXPECT_EQ(a.value(), expected);
}

TEST(BitVecTest, WideLiteral) {
  auto a = UInt<128>::Literal<42>();
  auto bytes = a.value();
  EXPECT_EQ(bytes[0], 42);  // LSB
  for (size_t i = 1; i < bytes.size(); ++i) {
    EXPECT_EQ(bytes[i], 0);
  }
}

TEST(BitVecTest, WideLiteralZero) {
  auto a = UInt<128>::Literal<0>();
  std::array<uint8_t, 16> expected{};
  EXPECT_EQ(a.value(), expected);
}

TEST(BitVecTest, WideCheckedFromIntegral) {
  auto [val, truncated] = UInt<128>::Checked(uint64_t{0xDEADBEEF});
  EXPECT_FALSE(truncated);
  auto bytes = val.value();
  EXPECT_EQ(bytes[0], 0xEF);
  EXPECT_EQ(bytes[1], 0xBE);
  EXPECT_EQ(bytes[2], 0xAD);
  EXPECT_EQ(bytes[3], 0xDE);
  for (size_t i = 4; i < bytes.size(); ++i) {
    EXPECT_EQ(bytes[i], 0);
  }
}

TEST(BitVecTest, WideCheckedNegativeOnUnsigned) {
  auto [val, truncated] = UInt<128>::Checked(-1);
  EXPECT_TRUE(truncated);
}

TEST(BitVecTest, WideCheckedFromBytesNoTruncation) {
  std::array<uint8_t, 16> bytes{};
  bytes[0] = 0xFF;
  bytes[15] = 0x01;
  auto [val, truncated] = UInt<128>::Checked(bytes);
  EXPECT_FALSE(truncated);
  EXPECT_EQ(val.value(), bytes);
}

TEST(BitVecTest, WideCheckedFromBytesWithTruncation) {
  // UInt<100>: 13 bytes, but only 4 bits used in byte[12].
  std::array<uint8_t, 13> bytes{};
  bytes[12] = 0xFF;  // Upper 4 bits are unused → truncation.
  auto [val, truncated] = UInt<100>::Checked(bytes);
  EXPECT_TRUE(truncated);
  auto result = val.value();
  EXPECT_EQ(result[12], 0x0F);  // Upper 4 bits zeroed.
}

TEST(BitVecTest, WideCheckedFromBytesExactFit) {
  // UInt<100>: 13 bytes, 4 bits used in byte[12].
  std::array<uint8_t, 13> bytes{};
  bytes[12] = 0x0F;  // Exactly fills the 4 used bits.
  auto [val, truncated] = UInt<100>::Checked(bytes);
  EXPECT_FALSE(truncated);
  EXPECT_EQ(val.value(), bytes);
}

}  // namespace
}  // namespace z3w
