// Snippets demonstrating Z3Wire's symbolic types: SymBool, SymUInt, SymSInt.
//
// Shows variable creation, compile-time range-checked literals, and passing
// symbolic expressions to a Z3 solver.
//
// Run: ./dev.sh bazel run //examples:types

#include <z3++.h>

#include "z3wire/sym_bit_vec.h"
#include "z3wire/sym_bool.h"

void demo_symbolic_types() {
  z3::context ctx;
  z3::solver solver(ctx);

  z3w::SymBool flag(ctx, "flag");
  z3w::SymUInt<10> address(ctx, "address");
  z3w::SymSInt<11> offset(ctx, "offset");

  auto t = z3w::SymBool::True(ctx);
  auto f = z3w::SymBool::False(ctx);

  auto a = z3w::SymUInt<8>::Literal<255>(ctx);  // OK

  // Compile error: value doesn't fit
  // auto b = z3w::SymUInt<8>::Literal<256>(ctx);

  auto c = z3w::SymSInt<8>::Literal<127>(ctx);  // OK

  // Compile error: value doesn't fit
  // auto d = z3w::SymSInt<8>::Literal<128>(ctx);

  z3w::SymUInt<8> x(ctx, "x");
  solver.add(x.expr() > 0);  // Pass to Z3 solver directly
}

int main() {
  demo_symbolic_types();
  return 0;
}
