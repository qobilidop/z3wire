#include <array>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>
#include <z3++.h>

#include "fuzztest/fuzztest.h"
#include "z3wire/bit_vec.h"
#include "z3wire/sym_bit_vec.h"

namespace z3w {
namespace {

// Property: Same-width signed/unsigned unsafe_cast roundtrip preserves bits.
// unsafe_cast<SymBitVec<W, S>>(unsafe_cast<SymBitVec<W, !S>>(v)) == v.
template <size_t W, bool S>
void VerifyCastRoundtrip(const BitVec<W, S>& original) {
  z3::context ctx;
  auto symbolic = to_symbolic(original, ctx);
  using Flipped = SymBitVec<W, !S>;
  using Same = SymBitVec<W, S>;
  auto roundtrip = unsafe_cast<Same>(unsafe_cast<Flipped>(symbolic));

  // Trivial model since the expression is already concrete.
  z3::solver solver(ctx);
  ASSERT_EQ(solver.check(), z3::sat);
  auto model = solver.get_model();

  EXPECT_EQ(original, to_concrete(roundtrip, model));
}

#define CAST_ROUNDTRIP_FUZZ_TEST(Type, W)                                 \
  void Type##W##CastRoundtrip(Type<W>::ValueType raw) {                   \
    /* TryFrom() masks raw to valid W-bit range. */                       \
    auto r = Type<W>::TryFrom(raw);                                       \
    VerifyCastRoundtrip(r.value);                                         \
  }                                                                       \
  FUZZ_TEST(SymBitVecCast, Type##W##CastRoundtrip)

CAST_ROUNDTRIP_FUZZ_TEST(UInt, 1);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 7);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 8);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 15);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 16);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 31);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 32);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 63);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 64);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 65);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 100);
CAST_ROUNDTRIP_FUZZ_TEST(UInt, 128);

CAST_ROUNDTRIP_FUZZ_TEST(SInt, 1);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 7);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 8);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 15);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 16);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 31);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 32);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 63);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 64);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 65);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 100);
CAST_ROUNDTRIP_FUZZ_TEST(SInt, 128);

}  // namespace
}  // namespace z3w
