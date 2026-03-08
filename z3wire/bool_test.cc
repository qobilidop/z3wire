#include "z3wire/bool.h"

#include <gtest/gtest.h>
#include <z3++.h>

namespace z3w {
namespace {

class BoolTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

TEST_F(BoolTest, SymbolicVariable) {
  Bool a(ctx_, "a");
  EXPECT_TRUE(a.raw().is_bool());
}

TEST_F(BoolTest, Literals) {
  Bool t = Bool::True(ctx_);
  Bool f = Bool::False(ctx_);
  EXPECT_TRUE(t.raw().is_true());
  EXPECT_TRUE(f.raw().is_false());
}

TEST_F(BoolTest, LogicalAnd) {
  Bool a(ctx_, "a");
  Bool b(ctx_, "b");
  Bool c = a && b;

  z3::solver s(ctx_);
  s.add(a.raw());
  s.add(!b.raw());
  s.add(c.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BoolTest, LogicalOr) {
  Bool a(ctx_, "a");
  Bool b(ctx_, "b");
  Bool c = a || b;

  z3::solver s(ctx_);
  s.add(!a.raw());
  s.add(!b.raw());
  s.add(c.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BoolTest, LogicalNot) {
  Bool a(ctx_, "a");
  Bool b = !a;

  z3::solver s(ctx_);
  s.add(a.raw());
  s.add(b.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BoolTest, Equality) {
  Bool a(ctx_, "a");
  Bool b(ctx_, "b");
  Bool eq = (a == b);

  // If a == b is true, and a is true, then b must be true.
  z3::solver s(ctx_);
  s.add(eq.raw());
  s.add(a.raw());
  s.add(!b.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(BoolTest, Inequality) {
  Bool a(ctx_, "a");
  Bool b(ctx_, "b");
  Bool neq = (a != b);

  // If a != b is true, and both are true, that's unsat.
  z3::solver s(ctx_);
  s.add(neq.raw());
  s.add(a.raw());
  s.add(b.raw());
  EXPECT_EQ(s.check(), z3::unsat);
}

}  // namespace
}  // namespace z3w
