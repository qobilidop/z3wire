#include <array>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "fuzztest/fuzztest.h"
#include "z3wire/bit_vec.h"

namespace z3w {
namespace {

// Helper to dispatch TryFrom call based on width.
template <size_t W, bool S>
typename BitVec<W, S>::TryFromResult TryFromValue(
    const typename BitVec<W, S>::ValueType& val) {
  if constexpr (W > 64) {
    return BitVec<W, S>::TryFromLeBytes(val);
  } else {
    return BitVec<W, S>::TryFrom(val);
  }
}

// Property: Checked(v.value()) on a valid BitVec returns {v, false}.
// A value produced by Checked() is already in range, so re-checking it
// must be idempotent with no truncation.
template <size_t W, bool S>
void VerifyCheckedIdempotent(const BitVec<W, S>& val) {
  auto result = TryFromValue<W, S>(val.value());
  EXPECT_EQ(val, result.value);
  EXPECT_FALSE(result.truncated);
}

#define CHECKED_IDEMPOTENT_FUZZ_TEST(Type, W)                             \
  void Type##W##CheckedIdempotent(Type<W>::ValueType raw) {               \
    /* TryFromValue() dispatches to TryFrom/TryFromLeBytes based on W. */ \
    auto r = TryFromValue<W, Type<W>::kIsSigned>(raw);                    \
    VerifyCheckedIdempotent(r.value);                                     \
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
