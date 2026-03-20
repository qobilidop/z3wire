#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include <gtest/gtest.h>

#include "fuzztest/fuzztest.h"
#include "z3wire/bit_vec.h"

namespace z3w {
namespace {

// Property: Checked(v.value()) on a valid BitVec returns {v, false}.
// A value produced by Checked() is already in range, so re-checking it
// must be idempotent with no truncation.
template <size_t W, bool S>
void VerifyCheckedIdempotent(const BitVec<W, S>& val) {
  auto [rechecked, truncated] = BitVec<W, S>::Checked(val.value());
  EXPECT_EQ(val, rechecked);
  EXPECT_FALSE(truncated);
}

#define CHECKED_IDEMPOTENT_FUZZ_TEST(Type, W)                             \
  void Type##W##CheckedIdempotent(Type<W>::ValueType raw) {               \
    /* Checked() masks raw to valid W-bit range. */                       \
    auto [val, truncated] = Type<W>::Checked(raw);                        \
    VerifyCheckedIdempotent(val);                                         \
  }                                                                       \
  FUZZ_TEST(BitVecChecked, Type##W##CheckedIdempotent)

CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 1);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 7);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 8);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 15);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 16);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 31);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 32);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 63);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 64);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 65);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 100);
CHECKED_IDEMPOTENT_FUZZ_TEST(UInt, 128);

CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 1);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 7);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 8);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 15);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 16);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 31);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 32);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 63);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 64);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 65);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 100);
CHECKED_IDEMPOTENT_FUZZ_TEST(SInt, 128);

}  // namespace
}  // namespace z3w
