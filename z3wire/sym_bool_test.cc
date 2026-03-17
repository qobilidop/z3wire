#include "z3wire/sym_bool.h"

#include <gtest/gtest.h>
#include <z3++.h>

#include "z3wire/bool.h"

namespace z3w {
namespace {

class SymBoolTest : public ::testing::Test {
 protected:
  z3::context ctx_;
};

TEST_F(SymBoolTest, SymbolicVariable) {
  SymBool a(ctx_, "a");
  EXPECT_TRUE(a.expr().is_bool());
}

TEST_F(SymBoolTest, Literals) {
  SymBool t = SymBool::True(ctx_);
  SymBool f = SymBool::False(ctx_);
  EXPECT_TRUE(t.expr().is_true());
  EXPECT_TRUE(f.expr().is_false());
}

TEST_F(SymBoolTest, LogicalAnd) {
  SymBool a(ctx_, "a");
  SymBool b(ctx_, "b");
  SymBool c = a && b;

  z3::solver s(ctx_);
  s.add(a.expr());
  s.add(!b.expr());
  s.add(c.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBoolTest, LogicalOr) {
  SymBool a(ctx_, "a");
  SymBool b(ctx_, "b");
  SymBool c = a || b;

  z3::solver s(ctx_);
  s.add(!a.expr());
  s.add(!b.expr());
  s.add(c.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBoolTest, LogicalNot) {
  SymBool a(ctx_, "a");
  SymBool b = !a;

  z3::solver s(ctx_);
  s.add(a.expr());
  s.add(b.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBoolTest, Xor) {
  SymBool a(ctx_, "a");
  SymBool b(ctx_, "b");
  SymBool c = a ^ b;

  // XOR: if both are true, result is false.
  z3::solver s(ctx_);
  s.add(a.expr());
  s.add(b.expr());
  s.add(c.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBoolTest, Equality) {
  SymBool a(ctx_, "a");
  SymBool b(ctx_, "b");
  SymBool eq = (a == b);

  // If a == b is true, and a is true, then b must be true.
  z3::solver s(ctx_);
  s.add(eq.expr());
  s.add(a.expr());
  s.add(!b.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBoolTest, Inequality) {
  SymBool a(ctx_, "a");
  SymBool b(ctx_, "b");
  SymBool neq = (a != b);

  // If a != b is true, and both are true, that's unsat.
  z3::solver s(ctx_);
  s.add(neq.expr());
  s.add(a.expr());
  s.add(b.expr());
  EXPECT_EQ(s.check(), z3::unsat);
}

TEST_F(SymBoolTest, ToSymbolicFromConcrete) {
  z3w::Bool concrete_true(true);
  z3w::Bool concrete_false(false);
  auto sym_t = z3w::to_symbolic(concrete_true, ctx_);
  auto sym_f = z3w::to_symbolic(concrete_false, ctx_);
  EXPECT_TRUE(sym_t.expr().is_true());
  EXPECT_TRUE(sym_f.expr().is_false());
}

}  // namespace
}  // namespace z3w
