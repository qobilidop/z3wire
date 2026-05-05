#include <cstdint>

#include <gtest/gtest.h>
#include <z3++.h>

#include "fuzztest/fuzztest.h"
#include "z3wire/sym_bit_vec.h"

namespace z3w {
namespace {

// Property: rotr(rotl(val, amt), amt) == val for any value and amount.
template <size_t W, size_t K>
void VerifyRotateRoundtrip(uint64_t raw_val, uint64_t raw_amt) {
  uint64_t val = (W < 64) ? (raw_val & ((uint64_t{1} << W) - 1)) : raw_val;
  uint64_t amt = (K < 64) ? (raw_amt & ((uint64_t{1} << K) - 1)) : raw_amt;

  z3::context ctx;
  auto sym_val = to_symbolic(UInt<W>::From(val), ctx);
  auto sym_amt = to_symbolic(UInt<K>::From(amt), ctx);

  auto rotated = rotl(sym_val, sym_amt);
  auto roundtripped = rotr(rotated, sym_amt);

  z3::solver solver(ctx);
  ASSERT_EQ(solver.check(), z3::sat);
  auto model = solver.get_model();

  auto result = to_concrete(roundtripped, model);
  EXPECT_EQ(result.value(), val)
      << "val=0x" << std::hex << val << " amt=" << std::dec << amt;
}

#define ROTATE_ROUNDTRIP_FUZZ_TEST(W, K)                        \
  void UInt##W##Rotate##K(uint64_t val, uint64_t amt) {         \
    VerifyRotateRoundtrip<W, K>(val, amt);                      \
  }                                                             \
  FUZZ_TEST(SymBitVecRotate, UInt##W##Rotate##K)

// K < W
ROTATE_ROUNDTRIP_FUZZ_TEST(8, 3);
ROTATE_ROUNDTRIP_FUZZ_TEST(16, 4);
ROTATE_ROUNDTRIP_FUZZ_TEST(32, 5);

// K == W
ROTATE_ROUNDTRIP_FUZZ_TEST(8, 8);
ROTATE_ROUNDTRIP_FUZZ_TEST(16, 16);

// K > W
ROTATE_ROUNDTRIP_FUZZ_TEST(8, 16);
ROTATE_ROUNDTRIP_FUZZ_TEST(16, 32);

}  // namespace
}  // namespace z3w
