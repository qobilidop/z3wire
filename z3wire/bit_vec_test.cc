#include "z3wire/bit_vec.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
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

// --- TryFrom (runtime, checked construction) ---

TEST(BitVecTest, TryFromUnsignedNegative) {
  auto r = UInt<8>::TryFrom(-1);
  EXPECT_TRUE(r.truncated);
}

TEST(BitVecTest, TryFromSignedPositive) {
  auto r = SInt<8>::TryFrom(127);
  EXPECT_EQ(r.value.value(), 127);
  EXPECT_FALSE(r.truncated);
}

TEST(BitVecTest, TryFromSignedOverflowNegative) {
  auto r = SInt<8>::TryFrom(-200);
  EXPECT_TRUE(r.truncated);
}

// --- TryFrom (runtime, returns TryFromResult) ---

TEST(BitVecTest, TryFromUnsignedFits) {
  auto r = UInt<8>::TryFrom(200);
  EXPECT_EQ(r.value.value(), 200);
  EXPECT_FALSE(r.truncated);
}

TEST(BitVecTest, TryFromUnsignedTruncates) {
  auto r = UInt<8>::TryFrom(300);
  EXPECT_EQ(r.value.value(), 44);  // 300 & 0xFF
  EXPECT_TRUE(r.truncated);
}

TEST(BitVecTest, TryFromSignedFits) {
  auto r = SInt<8>::TryFrom(-128);
  EXPECT_EQ(r.value.value(), -128);
  EXPECT_FALSE(r.truncated);
}

TEST(BitVecTest, TryFromSignedTruncates) {
  auto r = SInt<8>::TryFrom(200);
  EXPECT_EQ(r.value.value(), -56);  // bits 1100 1000 reinterpreted
  EXPECT_TRUE(r.truncated);
}

TEST(BitVecTest, TryFromWideFromInt) {
  auto r = UInt<128>::TryFrom(uint64_t{0xDEADBEEF});
  EXPECT_FALSE(r.truncated);
  EXPECT_EQ(r.value.ToLeBytes()[0], 0xEF);
  EXPECT_EQ(r.value.ToLeBytes()[1], 0xBE);
  EXPECT_EQ(r.value.ToLeBytes()[2], 0xAD);
  EXPECT_EQ(r.value.ToLeBytes()[3], 0xDE);
}

TEST(BitVecTest, TryFromWideFromBytes) {
  std::array<uint8_t, 16> bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                   0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC,
                                   0xDD, 0xEE, 0xFF, 0x00};
  auto r = UInt<128>::TryFromLeBytes(std::span<const uint8_t>{bytes});
  EXPECT_FALSE(r.truncated);
  EXPECT_EQ(r.value.ToLeBytes()[0], 0x11);
}

// --- From (runtime, aborts on truncation) ---

TEST(BitVecTest, FromUnsignedFits) {
  auto v = UInt<8>::From(200);
  EXPECT_EQ(v.value(), 200);
}

TEST(BitVecTest, FromSignedFits) {
  auto v = SInt<8>::From(-128);
  EXPECT_EQ(v.value(), -128);
}

TEST(BitVecTest, FromWideFromInt) {
  auto v = UInt<128>::From(uint64_t{0xDEADBEEF});
  EXPECT_EQ(v.ToLeBytes()[0], 0xEF);
}

TEST(BitVecTest, FromWideFromBytes) {
  std::array<uint8_t, 16> bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                   0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC,
                                   0xDD, 0xEE, 0xFF, 0x00};
  auto v = UInt<128>::FromLeBytes(std::span<const uint8_t>{bytes});
  EXPECT_EQ(v.ToLeBytes()[0], 0x11);
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST(BitVecDeathTest, FromUnsignedAbortsOnTruncation) {
  EXPECT_DEATH((void)UInt<8>::From(300), "construction value out of range");
}

TEST(BitVecDeathTest, FromSignedAbortsOnNegativeForUnsigned) {
  EXPECT_DEATH((void)UInt<8>::From(-1), "construction value out of range");
}

TEST(BitVecDeathTest, FromSignedAbortsOnOverflow) {
  EXPECT_DEATH((void)SInt<8>::From(200), "construction value out of range");
}
// NOLINTEND(readability-function-cognitive-complexity)

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

TEST(BitVecTest, TryFromWidth64MaxValue) {
  auto r = UInt<64>::TryFrom(UINT64_MAX);
  EXPECT_EQ(r.value.value(), UINT64_MAX);
  EXPECT_FALSE(r.truncated);
}

TEST(BitVecTest, TryFromWidth1Truncates) {
  auto r = UInt<1>::TryFrom(3);
  EXPECT_EQ(r.value.value(), 1);  // 3 & 0x1
  EXPECT_TRUE(r.truncated);
}

// --- Same-type equality ---

TEST(BitVecTest, SameTypeEquality) {
  auto a = UInt<8>::Literal<42>();
  auto b = UInt<8>::Literal<42>();
  auto c = UInt<8>::Literal<99>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a != c);
}

TEST(BitVecTest, SameTypeEqualitySigned) {
  auto a = SInt<8>::Literal<-1>();
  auto b = SInt<8>::Literal<-1>();
  auto c = SInt<8>::Literal<42>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
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
  EXPECT_EQ(a.ToLeBytes(), expected);
}

TEST(BitVecTest, WideLiteral) {
  auto a = UInt<128>::Literal<42>();
  auto bytes = a.ToLeBytes();
  EXPECT_EQ(bytes[0], 42);  // LSB
  for (size_t i = 1; i < bytes.size(); ++i) {
    EXPECT_EQ(bytes[i], 0);
  }
}

TEST(BitVecTest, WideLiteralZero) {
  auto a = UInt<128>::Literal<0>();
  std::array<uint8_t, 16> expected{};
  EXPECT_EQ(a.ToLeBytes(), expected);
}

TEST(BitVecTest, TryFromWideFromIntDeadbeef) {
  auto r = UInt<128>::TryFrom(uint64_t{0xDEADBEEF});
  EXPECT_FALSE(r.truncated);
  auto bytes = r.value.ToLeBytes();
  EXPECT_EQ(bytes[0], 0xEF);
  EXPECT_EQ(bytes[1], 0xBE);
  EXPECT_EQ(bytes[2], 0xAD);
  EXPECT_EQ(bytes[3], 0xDE);
  // Remaining bytes should be zero.
  std::array<uint8_t, 12> upper_bytes;
  std::copy(bytes.begin() + 4, bytes.end(), upper_bytes.begin());
  EXPECT_EQ(upper_bytes, (std::array<uint8_t, 12>{}));
}

TEST(BitVecTest, TryFromWideNegativeOnUnsigned) {
  auto r = UInt<128>::TryFrom(-1);
  EXPECT_TRUE(r.truncated);
}

TEST(BitVecTest, TryFromWideFromBytesNoTruncation) {
  std::array<uint8_t, 16> bytes{};
  bytes[0] = 0xFF;
  bytes[15] = 0x01;
  auto r = UInt<128>::TryFromLeBytes(bytes);
  EXPECT_FALSE(r.truncated);
  EXPECT_EQ(r.value.ToLeBytes(), bytes);
}

TEST(BitVecTest, TryFromWideFromBytesWithTruncation) {
  // UInt<100>: 13 bytes, but only 4 bits used in byte[12].
  std::array<uint8_t, 13> bytes{};
  bytes[12] = 0xFF;  // Upper 4 bits are unused → truncation.
  auto r = UInt<100>::TryFromLeBytes(bytes);
  EXPECT_TRUE(r.truncated);
  auto result = r.value.ToLeBytes();
  EXPECT_EQ(result[12], 0x0F);  // Upper 4 bits zeroed.
}

TEST(BitVecTest, TryFromWideFromBytesExactFit) {
  // UInt<100>: 13 bytes, 4 bits used in byte[12].
  std::array<uint8_t, 13> bytes{};
  bytes[12] = 0x0F;  // Exactly fills the 4 used bits.
  auto r = UInt<100>::TryFromLeBytes(bytes);
  EXPECT_FALSE(r.truncated);
  EXPECT_EQ(r.value.ToLeBytes(), bytes);
}

TEST(BitVecTest, TryFromWideFromShorterSpan) {
  // UInt<128>: 16 bytes. Pass only 2 bytes — rest should be zero-padded.
  std::array<uint8_t, 2> bytes{0xAB, 0xCD};
  auto r = UInt<128>::TryFromLeBytes(bytes);
  EXPECT_FALSE(r.truncated);
  auto result = r.value.ToLeBytes();
  EXPECT_EQ(result[0], 0xAB);
  EXPECT_EQ(result[1], 0xCD);
  for (size_t i = 2; i < 16; ++i) {
    EXPECT_EQ(result[i], 0);
  }
}

TEST(BitVecTest, TryFromWideFromLongerSpanNoTruncation) {
  // UInt<128>: 16 bytes. Pass 20 bytes with trailing zeros — no truncation.
  std::array<uint8_t, 20> bytes{};
  bytes[0] = 0xFF;
  auto r = UInt<128>::TryFromLeBytes(bytes);
  EXPECT_FALSE(r.truncated);
  EXPECT_EQ(r.value.ToLeBytes()[0], 0xFF);
}

TEST(BitVecTest, TryFromWideFromLongerSpanWithTruncation) {
  // UInt<128>: 16 bytes. Pass 20 bytes with non-zero extra — truncation.
  std::array<uint8_t, 20> bytes{};
  bytes[18] = 0x01;
  auto r = UInt<128>::TryFromLeBytes(bytes);
  EXPECT_TRUE(r.truncated);
}

TEST(BitVecTest, ToLeBytesWideRoundtrip) {
  std::array<uint8_t, 16> bytes{};
  bytes[0] = 0x11;
  bytes[7] = 0x88;
  bytes[15] = 0xFF;
  auto v = UInt<128>::FromLeBytes(bytes);
  EXPECT_EQ(v.ToLeBytes(), bytes);
}

TEST(BitVecTest, WideEquality) {
  auto a = UInt<128>::Literal<42>();
  auto b = UInt<128>::Literal<42>();
  auto c = UInt<128>::Literal<99>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(BitVecTest, WideInequality) {
  auto a = UInt<128>::Literal<42>();
  auto b = UInt<128>::Literal<99>();
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a != a);
}

TEST(BitVecTest, WideDefaultIsZero) {
  UInt<128> a;
  auto b = UInt<128>::Literal<0>();
  EXPECT_TRUE(a == b);
}

TEST(BitVecTest, WideSignedEquality) {
  auto a = SInt<128>::Literal<42>();
  auto b = SInt<128>::Literal<42>();
  auto c = SInt<128>::Literal<99>();
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a != c);
}

}  // namespace
}  // namespace z3w
