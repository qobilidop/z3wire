#include "z3wire/check.h"

#include <gtest/gtest.h>

namespace z3w {
namespace {

// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST(CheckDeathTest, FailsWithConditionText) {
  EXPECT_DEATH(Z3W_CHECK(false), "Z3Wire check failed: false");
}

TEST(CheckDeathTest, FailsWithStreamedMessage) {
  EXPECT_DEATH(Z3W_CHECK(1 == 2) << "extra info", "extra info");
}
// NOLINTEND(readability-function-cognitive-complexity)

TEST(CheckTest, PassesOnTrueCondition) {
  Z3W_CHECK(true);
  Z3W_CHECK(1 == 1);
}

}  // namespace
}  // namespace z3w
