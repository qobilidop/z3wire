#include <cstdint>

#include <gtest/gtest.h>
#include <z3++.h>

#include "fuzztest/fuzztest.h"
#include "z3wire/sym_bit_vec.h"

namespace z3w {
namespace {

// Property: symbolic-offset extract on concrete values matches C++ bit
// arithmetic. For unsigned values, (val >> offset) is a logical right shift.
//
// Template parameters:
//   W       - source bit-vector width
//   TW      - target (extracted) width
//   IdxW    - index bit-vector width
template <size_t W, size_t TW, size_t IdxW>
void VerifyExtract(uint64_t raw_val, uint64_t raw_offset) {
  // Mask inputs to valid ranges.
  uint64_t val = (W < 64) ? (raw_val & ((uint64_t{1} << W) - 1)) : raw_val;
  uint64_t offset = (IdxW < 64) ? (raw_offset & ((uint64_t{1} << IdxW) - 1))
                                : raw_offset;

  // Reference: C++ logical right shift then mask.
  uint64_t expected = (offset >= W) ? 0 : (val >> offset);
  if constexpr (TW < 64) {
    expected &= (uint64_t{1} << TW) - 1;
  }

  // Symbolic computation.
  z3::context ctx;
  auto sym_val = to_symbolic(std::get<0>(UInt<W>::FromValue(val)), ctx);
  auto sym_idx = to_symbolic(std::get<0>(UInt<IdxW>::FromValue(offset)), ctx);
  auto sym_result = extract<TW>(sym_val, sym_idx);

  z3::solver solver(ctx);
  ASSERT_EQ(solver.check(), z3::sat);
  auto model = solver.get_model();
  auto concrete_result = to_concrete(sym_result, model);

  EXPECT_EQ(concrete_result.value(), expected)
      << "val=0x" << std::hex << val << " offset=" << std::dec << offset;
}

// --- IdxW < W ---

// extract<4> from UInt<8> with UInt<3> index.
void UInt8Extract4Idx3(uint64_t val, uint64_t offset) {
  VerifyExtract<8, 4, 3>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt8Extract4Idx3);

// extract<8> from UInt<16> with UInt<4> index.
void UInt16Extract8Idx4(uint64_t val, uint64_t offset) {
  VerifyExtract<16, 8, 4>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt16Extract8Idx4);

// extract<16> from UInt<32> with UInt<5> index.
void UInt32Extract16Idx5(uint64_t val, uint64_t offset) {
  VerifyExtract<32, 16, 5>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt32Extract16Idx5);

// --- IdxW == W ---

// extract<4> from UInt<8> with UInt<8> index.
void UInt8Extract4Idx8(uint64_t val, uint64_t offset) {
  VerifyExtract<8, 4, 8>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt8Extract4Idx8);

// extract<8> from UInt<16> with UInt<16> index.
void UInt16Extract8Idx16(uint64_t val, uint64_t offset) {
  VerifyExtract<16, 8, 16>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt16Extract8Idx16);

// --- IdxW > W ---

// extract<4> from UInt<8> with UInt<16> index.
void UInt8Extract4Idx16(uint64_t val, uint64_t offset) {
  VerifyExtract<8, 4, 16>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt8Extract4Idx16);

// extract<8> from UInt<16> with UInt<32> index.
void UInt16Extract8Idx32(uint64_t val, uint64_t offset) {
  VerifyExtract<16, 8, 32>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt16Extract8Idx32);

// --- Edge cases: TW == W (extract full width) ---

// extract<8> from UInt<8> with UInt<4> index.
void UInt8Extract8Idx4(uint64_t val, uint64_t offset) {
  VerifyExtract<8, 8, 4>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, UInt8Extract8Idx4);

// --- Signed source ---

// extract<4> from SInt<8> with UInt<4> index.
// Extract always returns SymUInt, so we compare against unsigned bit arithmetic.
template <size_t W, size_t TW, size_t IdxW>
void VerifyExtractSigned(uint64_t raw_val, uint64_t raw_offset) {
  uint64_t val = (W < 64) ? (raw_val & ((uint64_t{1} << W) - 1)) : raw_val;
  uint64_t offset = (IdxW < 64) ? (raw_offset & ((uint64_t{1} << IdxW) - 1))
                                : raw_offset;

  // Reference: treat source as unsigned bit pattern for logical right shift.
  uint64_t expected = (offset >= W) ? 0 : (val >> offset);
  if constexpr (TW < 64) {
    expected &= (uint64_t{1} << TW) - 1;
  }

  z3::context ctx;
  // Create signed symbolic value from unsigned bit pattern.
  auto sym_val =
      SymSInt<W>(ctx.bv_val(static_cast<uint64_t>(val), W));
  auto sym_idx = to_symbolic(std::get<0>(UInt<IdxW>::FromValue(offset)), ctx);
  auto sym_result = extract<TW>(sym_val, sym_idx);

  z3::solver solver(ctx);
  ASSERT_EQ(solver.check(), z3::sat);
  auto model = solver.get_model();
  auto concrete_result = to_concrete(sym_result, model);

  EXPECT_EQ(concrete_result.value(), expected)
      << "val=0x" << std::hex << val << " offset=" << std::dec << offset;
}

void SInt8Extract4Idx4(uint64_t val, uint64_t offset) {
  VerifyExtractSigned<8, 4, 4>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, SInt8Extract4Idx4);

void SInt16Extract8Idx16(uint64_t val, uint64_t offset) {
  VerifyExtractSigned<16, 8, 16>(val, offset);
}
FUZZ_TEST(SymBitVecExtract, SInt16Extract8Idx16);

}  // namespace
}  // namespace z3w
