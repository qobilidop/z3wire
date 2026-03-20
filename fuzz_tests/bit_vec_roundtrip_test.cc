#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include <gtest/gtest.h>
#include <z3++.h>

#include "fuzztest/fuzztest.h"
#include "z3wire/bit_vec.h"
#include "z3wire/sym_bit_vec.h"

namespace z3w {
namespace {

// Helper: roundtrip a concrete BitVec through symbolic and back.
template <size_t W, bool S>
void VerifyRoundtrip(const BitVec<W, S>& original) {
  z3::context ctx;
  auto symbolic = to_symbolic(original, ctx);

  // Create a trivial model to evaluate the (already-concrete) expression.
  z3::solver solver(ctx);
  ASSERT_EQ(solver.check(), z3::sat);
  auto model = solver.get_model();

  auto roundtripped = to_concrete(symbolic, model);
  EXPECT_EQ(original, roundtripped);
}

#define ROUNDTRIP_FUZZ_TEST(Type, W)                                      \
  void Type##W##Roundtrip(Type<W>::ValueType raw) {                       \
    /* Checked() masks raw to valid W-bit range. */                       \
    auto [val, truncated] = Type<W>::Checked(raw);                        \
    VerifyRoundtrip(val);                                                 \
  }                                                                       \
  FUZZ_TEST(BitVecRoundtrip, Type##W##Roundtrip)

ROUNDTRIP_FUZZ_TEST(UInt, 1);
ROUNDTRIP_FUZZ_TEST(UInt, 7);
ROUNDTRIP_FUZZ_TEST(UInt, 8);
ROUNDTRIP_FUZZ_TEST(UInt, 15);
ROUNDTRIP_FUZZ_TEST(UInt, 16);
ROUNDTRIP_FUZZ_TEST(UInt, 31);
ROUNDTRIP_FUZZ_TEST(UInt, 32);
ROUNDTRIP_FUZZ_TEST(UInt, 63);
ROUNDTRIP_FUZZ_TEST(UInt, 64);
ROUNDTRIP_FUZZ_TEST(UInt, 65);
ROUNDTRIP_FUZZ_TEST(UInt, 100);
ROUNDTRIP_FUZZ_TEST(UInt, 128);

ROUNDTRIP_FUZZ_TEST(SInt, 1);
ROUNDTRIP_FUZZ_TEST(SInt, 7);
ROUNDTRIP_FUZZ_TEST(SInt, 8);
ROUNDTRIP_FUZZ_TEST(SInt, 15);
ROUNDTRIP_FUZZ_TEST(SInt, 16);
ROUNDTRIP_FUZZ_TEST(SInt, 31);
ROUNDTRIP_FUZZ_TEST(SInt, 32);
ROUNDTRIP_FUZZ_TEST(SInt, 63);
ROUNDTRIP_FUZZ_TEST(SInt, 64);
ROUNDTRIP_FUZZ_TEST(SInt, 65);
ROUNDTRIP_FUZZ_TEST(SInt, 100);
ROUNDTRIP_FUZZ_TEST(SInt, 128);

}  // namespace
}  // namespace z3w
