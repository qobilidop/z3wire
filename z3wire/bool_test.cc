#include "z3wire/bool.h"

#include <sstream>

#include <gtest/gtest.h>

namespace z3w {
namespace {

TEST(BoolTest, DefaultConstructor) {
  Bool b;
  EXPECT_EQ(b.value(), false);
}

TEST(BoolTest, ConstructFromTrue) {
  Bool b(true);
  EXPECT_EQ(b.value(), true);
}

TEST(BoolTest, ConstructFromFalse) {
  Bool b(false);
  EXPECT_EQ(b.value(), false);
}

TEST(BoolTest, ImplicitFromBoolLiteral) {
  Bool b = true;
  EXPECT_EQ(b.value(), true);
}

TEST(BoolTest, ExplicitOperatorBool) {
  Bool b(true);
  EXPECT_TRUE(static_cast<bool>(b));
}

TEST(BoolTest, ContextualConversion) {
  Bool b(true);
  // explicit operator bool() allows contextual conversion.
  if (b) {
    SUCCEED();
  } else {
    ADD_FAILURE();
  }
}

TEST(BoolTest, EqualityBoolBool) {
  EXPECT_EQ(Bool(true), Bool(true));
  EXPECT_EQ(Bool(false), Bool(false));
  EXPECT_NE(Bool(true), Bool(false));
}

TEST(BoolTest, EqualityBoolNative) {
  EXPECT_EQ(Bool(true), true);
  EXPECT_EQ(true, Bool(true));
  EXPECT_EQ(Bool(false), false);
  EXPECT_NE(Bool(true), false);
}

TEST(BoolTest, ValueAccessor) {
  EXPECT_TRUE(Bool(true).value());
  EXPECT_FALSE(Bool(false).value());
}

TEST(BoolTest, StreamOutput) {
  std::ostringstream os;
  os << Bool(true) << " " << Bool(false);
  EXPECT_EQ(os.str(), "true false");
}

TEST(BoolTest, Constexpr) {
  constexpr Bool b(true);
  static_assert(b.value() == true);
  constexpr Bool d;
  static_assert(d.value() == false);
}

}  // namespace
}  // namespace z3w
