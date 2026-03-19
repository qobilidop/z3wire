// Snippets demonstrating Z3Wire's type system: symbolic and concrete types.
//
// Shows variable creation, compile-time range-checked literals, Z3 interop,
// concrete construction, and value access.
//
// Run: ./dev.sh bazel run //examples/usage:types

#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/bool.h"
#include "z3wire/sym_bit_vec.h"
#include "z3wire/sym_bool.h"

// --- Symbolic types ---

void demo_symbolic_construction() {
  z3::context ctx;

  z3w::SymBool flag(ctx, "flag");
  z3w::SymUInt<8> x(ctx, "x");
  z3w::SymSInt<8> y(ctx, "y");

  auto t = z3w::SymBool::True(ctx);
  auto f = z3w::SymBool::False(ctx);

  auto a = z3w::SymUInt<8>::Literal<255>(ctx);  // OK
  auto c = z3w::SymSInt<8>::Literal<127>(ctx);  // OK

  // Compile error: value doesn't fit
  // auto b = z3w::SymUInt<8>::Literal<256>(ctx);
  // auto d = z3w::SymSInt<8>::Literal<128>(ctx);
}

void demo_z3_interop() {
  z3::context ctx;
  z3::solver solver(ctx);
  z3w::SymUInt<8> x(ctx, "x");
  solver.add(x.expr() > 0);  // Pass to Z3 solver directly
}

// --- Concrete types ---

void demo_concrete_construction() {
  z3w::Bool flag = true;  // OK: implicit from bool

  // Compile error: deleted integral constructor
  // z3w::Bool flag2 = 42;

  auto a = z3w::UInt<8>::Literal<255>();  // OK
  auto c = z3w::SInt<8>::Literal<127>();  // OK

  // Compile error: value doesn't fit
  // auto b = z3w::UInt<8>::Literal<256>();
  // auto d = z3w::SInt<8>::Literal<128>();

  auto [x, x_truncated] = z3w::UInt<8>::Checked(300);
  // x.value() == 44, x_truncated == true
  auto [y, y_truncated] = z3w::UInt<8>::Checked(200);
  // y.value() == 200, y_truncated == false
}

void demo_value_access() {
  z3w::Bool flag = true;
  bool flag_val = flag.value();  // true

  auto x = z3w::UInt<8>::Literal<200>();
  auto x_val = x.value();  // 200 (uint8_t)

  auto y = z3w::SInt<8>::Literal<-1>();
  auto y_val = y.value();  // -1 (int8_t)

  if (flag) { /* contextual conversion to bool */
  }
}

int main() {
  demo_symbolic_construction();
  demo_z3_interop();
  demo_concrete_construction();
  demo_value_access();
  return 0;
}
